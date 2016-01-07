// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var debug = require('debug')('azure:event-hubs:processor:partition');
var Promise = require('bluebird');
var uuid = require('uuid');

function PartitionContext(partitionId, owner, lease) {
  this.partitionId = partitionId;
  this._owner = owner;
  this.lease = lease;
  this._token = uuid.v4();
  var self = this;
  this._checkpointDetails = {
    PartitionId: self.partitionId,
    Owner: self._owner,
    Token: self._token,
    Epoch: 1,
    Offset: '',
    SequenceNumber: 0
  };
}

/**
 * Stores the checkpoint data into the appropriate blob, assuming the lease is held (otherwise, rejects).
 *
 * The checkpoint data is compatible with the .NET EventProcessorHost, and is structured as a JSON payload (example):
 * {"PartitionId":"0","Owner":"ephtest","Token":"48e209e3-55f0-41b8-a8dd-d9c09ff6c35a","Epoch":1,"Offset":"","SequenceNumber":0}
 *
 * @method checkpoint
 *
 * @return {Promise}
 */
PartitionContext.prototype.checkpoint = function() {
  var self = this;
  return new Promise(function (resolve, reject) {
    if (self.lease.isHeld()) {
      self._checkpointDetails.Owner = self._owner; // We're setting it, ensure we're the owner.
      self.lease.updateContents(JSON.stringify(self._checkpointDetails))
        .then(function() { resolve(self._checkpointDetails); }).catch(reject);
    } else {
      reject(new Error('Lease not held'));
    }
  });
};

PartitionContext.prototype.setCheckpointData = function(owner, token, epoch, offset, sequenceNumber) {
  this._checkpointDetails.Owner = owner;
  this._checkpointDetails.Token = token;
  this._checkpointDetails.Epoch = epoch;
  this._checkpointDetails.Offset = offset;
  this._checkpointDetails.SequenceNumber = sequenceNumber;
};

PartitionContext.prototype.setCheckpointDataFromPayload = function(payload) {
  this._checkpointDetails = payload;
};

PartitionContext.prototype.updateCheckpointDataFromLease = function() {
  var self = this;
  return new Promise(function (resolve, reject) {
    self.lease.getContents().then(function (contents) {
      if (contents) {
        debug('Lease '+self.lease.fullUri+' contents: ' + contents);
        try {
          var payload = JSON.parse(contents);
          self.setCheckpointDataFromPayload(payload);
        } catch (err) {
          reject('Invalid payload "' + contents + '": ' + err);
          return;
        }
      }
      resolve(self._checkpointDetails);
    }).catch(reject);
  });
};

/**
 * Updates data from the message, which should have an annotations field containing something like:
 *   "x-opt-sequence-number":6,"x-opt-offset":"480","x-opt-enqueued-time":"2015-12-18T17:26:49.331Z"
 *
 * @method updateCheckpointDataFromMessage
 * @param {*} message
 */
PartitionContext.prototype.updateCheckpointDataFromMessage = function(message) {
  if (message && message.annotations && message.annotations.value) {
    var anno = message.annotations.value;
    if (anno['x-opt-enqueued-time']) this._checkpointDetails.Epoch = anno['x-opt-enqueued-time'];
    if (anno['x-opt-offset']) this._checkpointDetails.Offset = anno['x-opt-offset'];
    if (anno['x-opt-sequence-number']) this._checkpointDetails.SequenceNumber = anno['x-opt-sequence-number'];
  }
};

module.exports = PartitionContext;
