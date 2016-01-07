// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var debug = require('debug')('azure:event-hubs:processor:host');
var util = require('util');
var Promise = require('bluebird');
var EventEmitter = require('events').EventEmitter;
var Cerulean = require('cerulean');
var ArgumentError = require('azure-iot-common').errors.ArgumentError;
var EventHubClient = require('azure-event-hubs').Client;
var PartitionContext = require('./partition_context');

function EventProcessorHost(name, consumerGroup, storageConnectionString, eventHubClient) {
  var ensure = function (paramName, param) {
    if (!param) throw new ArgumentError(paramName + ' cannot be null or missing');
  };
  ensure('name', name);
  ensure('consumerGroup', consumerGroup);
  ensure('storageConnectionString', storageConnectionString);
  ensure('eventHubClient', eventHubClient);

  this._hostName = name;
  this._consumerGroup = consumerGroup;
  this._eventHubClient = eventHubClient;
  this._storageConnectionString = storageConnectionString;
}

util.inherits(EventProcessorHost, EventEmitter);

// Events
// Opened: Triggered whenever a partition obtains its lease. Passed the PartitionContext.
EventProcessorHost.Opened = 'ephost:opened';
// Closed: Triggered whenever a partition loses its lease and has to stop receiving, or when the the host is shut down.
//         Passed the PartitionContext and the closing reason.
EventProcessorHost.Closed = 'ephost:closed';
// Message: Triggered whenever a message comes in on a given partition. Passed the PartitionContext and a message.
EventProcessorHost.Message = 'ephost:message';

EventProcessorHost.fromConnectionString = function (name, consumerGroup, storageConnectionString, eventHubConnectionString, eventHubPath) {
  return new EventProcessorHost(name, consumerGroup, storageConnectionString, EventHubClient.fromConnectionString(eventHubConnectionString, eventHubPath));
};

/**
 * Starts the event processor host, fetching the list of partitions, (optionally) filtering them, and attempting
 * to grab leases on the (filtered) set. For each successful lease, will get the details from the blob and start
 * a receiver at the point where it left off previously.
 *
 * @method start
 * @param {function} [partitionFilter]  Predicate that takes a partition ID and return true/false for whether we should
 *  attempt to grab the lease and watch it. If not provided, all partitions will be tried.
 *
 * @return {Promise}
 */
EventProcessorHost.prototype.start = function(partitionFilter) {
  var self = this;
  return new Promise(function (resolve, reject) {
    self._contextByPartition = {};
    self._receiverByPartition = {};
    self._leaseManager = new Cerulean.LeaseManager();
    self._leaseManager.on(Cerulean.LeaseManager.Acquired, function(_lease) {
      debug('Acquired lease on ' + _lease.partitionId);
      self._attachReceiver(_lease.partitionId);
    });
    self._leaseManager.on(Cerulean.LeaseManager.Lost, function(_lease) {
      debug('Lost lease on ' + _lease.partitionId);
      self._detachReceiver(_lease.partitionId, 'Lease lost');
    });
    self._leaseManager.on(Cerulean.LeaseManager.Released, function(_lease) {
      debug('Released lease on ' + _lease.partitionId);
      self._detachReceiver(_lease.partitionId, 'Lease released');
    });
    self._eventHubClient.getPartitionIds()
      .then(function (ids) {
        ids.forEach(function (id) {
          debug('Managing lease for partition ' + id);
          if (partitionFilter && !partitionFilter(id)) return; // Skip this partition

          var blobPath = self._consumerGroup + '/' + id;
          var lease = new Cerulean.Lease(self._storageConnectionString, self._hostName, blobPath);
          lease.partitionId = id;
          self._contextByPartition[id] = new PartitionContext(id, self._hostName, lease);
          self._leaseManager.manageLease(lease);
        });
        resolve(self);
      }).catch(reject);
  });
};

EventProcessorHost.prototype.stop = function() {
  var self = this;
  var unmanage = function(l) { return self._leaseManager.unmanageLease(l); };
  var releases = [];
  for (var partitionId in self._contextByPartition) {
    if (!self._contextByPartition.hasOwnProperty(partitionId)) continue;
    var id = partitionId;
    var context = self._contextByPartition[id];
    releases.push(self._detachReceiver(id).then(unmanage.bind(null, context.lease)));
  }
  return Promise.all(releases).then(function() {
    self._leaseManager = null;
    self._contextByPartition = {};
  });
};

EventProcessorHost.prototype._attachReceiver = function(partitionId) {
  var self = this;
  var context = self._contextByPartition[partitionId];
  if (!context) return Promise.reject(new Error('Invalid state - missing context for partition ' + partitionId));

  return context.updateCheckpointDataFromLease().then(function (checkpoint) {
    var filterOptions = null;
    if (checkpoint && checkpoint.Offset) {
      filterOptions = { startAfterOffset: checkpoint.Offset };
    }
    debug('Attaching receiver for '+partitionId+' with offset: ' + (checkpoint ? checkpoint.Offset : 'None'));
    return self._eventHubClient.createReceiver(self._consumerGroup, partitionId, filterOptions);
  }).then(function (receiver) {
    self.emit(EventProcessorHost.Opened, context);
    self._receiverByPartition[partitionId] = receiver;
    receiver.on('message', function (message) {
      context.updateCheckpointDataFromMessage(message);
      self.emit(EventProcessorHost.Message, context, message);
    });
    return receiver;
  });
};

EventProcessorHost.prototype._detachReceiver = function(partitionId, reason) {
  var self = this;
  var context = self._contextByPartition[partitionId];
  var receiver = self._receiverByPartition[partitionId];
  if (receiver) {
    delete self._receiverByPartition[partitionId];
    return receiver.close().then(function () {
      self.emit(EventProcessorHost.Closed, context, reason);
    });
  } else return Promise.resolve();
};

module.exports = EventProcessorHost;