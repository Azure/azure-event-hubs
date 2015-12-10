// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var EventEmitter = require('events');
var util = require('util');

var errors = require('./errors');

function EventHubSender(amqpSenderLink) {
  var self = this;
  self._senderLink = amqpSenderLink;

  EventEmitter.call(self);

  var onErrorReceived = function (err) {
    self.emit('errorReceived', errors.translate(err));
  };

  self.on('newListener', function (event) {
    if (event === 'errorReceived') {
      self._senderLink.on('errorReceived', onErrorReceived);
    }
  });

  self.on('removeListener', function (event) {
    if (event === 'errorReceived') {
      self._senderLink.removeListener('errorReceived', onErrorReceived);
    }
  });
}

/**
 * Sends the given message, with the given options on this link
 *
 * @method send
 * @param {*} message                   Message to send.  Will be sent as UTF8-encoded JSON string.
 * @param {string} [partitionKey]       Partition key - sent as x-opt-partition-key, and will hash to a partition ID.
 *
 * @return {Promise}
 */
EventHubSender.prototype.send = function (message, partitionKey) {
  var options = null;
  if (partitionKey) {
    options = {annotations: {'x-opt-partition-key': partitionKey}};
  }
  return this._senderLink.send(message, options);
};

util.inherits(EventHubSender, EventEmitter);

module.exports = EventHubSender;
