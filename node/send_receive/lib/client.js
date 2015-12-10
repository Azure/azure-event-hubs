// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var uuid = require('uuid');
var amqp10 = require('amqp10');
var Promise = require('bluebird');
var Receiver = require('./receiver');
var Sender = require('./sender');
var ConnectionConfig = require('./config');
var ArgumentError = require('azure-iot-common').errors.ArgumentError;
var MessagingEntityNotFoundError = require('./errors').MessagingEntityNotFoundError;

function EventHubClient(config) {
  var makeError = function (prop) {
    return new ArgumentError('config is missing property ' + prop);
  };

  ['host', 'path', 'keyName', 'key'].forEach(function (prop) {
    if (!config[prop]) throw makeError(prop);
  });
  
  this._uri = config.saslPlainUri();
  this._eventHubPath = config.path;
  this._amqp = new amqp10.Client(amqp10.Policy.EventHub);
  this._connectPromise = null;
}

EventHubClient.fromConnectionString = function (connectionString, path) {
  if (!connectionString) {
    throw new ArgumentError('Missing argument connectionString');
  }
  
  var config = new ConnectionConfig(connectionString, path);
  if (!config.path) {
    throw new ArgumentError('Connection string doesn\'t have EntityPath, or missing argument path');
  }

  return new EventHubClient(config);
};

EventHubClient.prototype.open = function () {
  if (!this._connectPromise) {
    this._connectPromise = this._amqp.connect(this._uri);
  }
  
  return this._connectPromise;
};

EventHubClient.prototype.close = function () {
  this._connectPromise = null;
  return this._amqp.disconnect();
};

EventHubClient.prototype.getPartitionIds = function () {
  return new Promise(function (resolve, reject) {
    var endpoint = '$management';
    var replyTo = uuid.v4();

    var request = {
      body: [],
      properties: {
        messageId: uuid.v4(),
        replyTo: replyTo
      },
      applicationProperties: {
        operation: "READ",
        name: this._eventHubPath,
        type: "com.microsoft:eventhub"
      }
    };

    var rxopt = { attach: { target: { address: replyTo } } };

    this.open()
      .then(function () {    
        return Promise.all([
          this._amqp.createReceiver(endpoint, rxopt),
          this._amqp.createSender(endpoint)
        ]);
      }.bind(this))
      .spread(function (receiver, sender) {
        receiver.on('errorReceived', reject);
        sender.on('errorReceived', reject);

        receiver.on('message', function (msg) {
          var code = msg.applicationProperties.value['status-code'];
          var desc = msg.applicationProperties.value['status-description'];
          if (code === 200) {
            resolve(msg.body.partition_ids);
          }
          else if (code === 404) {
            reject(new MessagingEntityNotFoundError(desc));
          }
        });

        return sender.send(request);
      });
  }.bind(this));
};

/**
 * Creates a receiver for the given event hub, consumer group, and partition.
 * Receivers are event emitters, watch for 'message' and 'errorReceived' events.
 *
 * @method createReceiver
 * @param {string} consumerGroup                      Consumer group from which to receive.
 * @param {(string|Number)} partitionId               Partition ID from which to receive.
 * @param {*} [options]                               Options for how you'd like to connect. Only one can be specified.
 * @param {(Date|Number)} options.startAfterTime      Only receive messages enqueued after the given time.
 * @param {string} options.startAfterOffset           Only receive messages after the given offset.
 * @param {string} options.customFilter               If you want more fine-grained control of the filtering.
 *      See https://github.com/Azure/amqpnetlite/wiki/Azure%20Service%20Bus%20Event%20Hubs for details.
 *
 * @return {Promise}
 */
EventHubClient.prototype.createReceiver = function createReceiver(consumerGroup, partitionId, options) {
  var self = this;
  return this.open()
    .then(function () {
      var endpoint = '/' + self._eventHubPath +
        '/ConsumerGroups/' + consumerGroup +
        '/Partitions/' + partitionId;
      var options = null;
      if (options) {
        var filterClause = null;
        if (options.startAfterTime) {
          var time = (options.startAfterTime instanceof Date) ? options.startAfterTime.getTime() : options.startAfterTime;
          filterClause = "amqp.annotation.x-opt-enqueuedtimeutc > '" + time + "'";
        } else if (options.startAfterOffset) {
          filterClause = "amqp.annotation.x-opt-offset > '" + options.startAfterOffset + "'";
        } else if (options.customFilter) {
          filterClause = options.customFilter;
        }

        if (filterClause) {
          options = {
            attach: { source: { filter: {
              'apache.org:selector-filter:string': amqp10.translator(
                ['described', ['symbol', 'apache.org:selector-filter:string'], ['string', filterClause]])
            } } }
          };
        }
      }

      return self._amqp.createReceiver(endpoint, options);
    })
    .then(function (amqpReceiver) {
      return new Receiver(amqpReceiver);
    });
};

/**
 * Creates a sender to the given event hub, and optionally to a given partition.
 * Senders are event emitters, watch for 'errorReceived' events.
 *
 * @method createSender
 * @param {(string|Number)} [partitionId]                 Partition ID from which to receive.
 *
 * @return {Promise}
 */
EventHubClient.prototype.createSender = function createSender(partitionId) {
  var self = this;
  return this.open()
    .then(function () {
      var endpoint = '/' + self._eventHubPath;
      if (partitionId) {
        endpoint += '/Partitions/' + partitionId;
      }
      return self._amqp.createSender(endpoint);
    })
    .then(function (amqpSender) {
      return new Sender(amqpSender);
    });
};

module.exports = EventHubClient;