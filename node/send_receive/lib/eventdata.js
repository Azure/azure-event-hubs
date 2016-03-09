// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

/**
 * @class EventData
 * @classdesc Constructs a {@linkcode EventData} object.
 * @param {object} link The receiver link for providing settlment methods.
 * @param {object}  message The message object containing body and messageAnnotations.
 */
function EventData(link, message) {
  Object.defineProperties(this, {
    'partitionKey': {
      get: function () {
        if (this.systemProperties) {
          return this.systemProperties["x-opt-partition-key"];
        } else {
          return null;
        }
      }
    },
    'body': {
      value: message.body,
      writable: false
    },
    'enqueuedTimeUtc': {
      get: function () {
        if (this.systemProperties) {
          return this.systemProperties["x-opt-enqueued-time"];
        } else {
          return null;
        }
      }
    },
    'offset': {
      get: function () {
        if (this.systemProperties) {
          return this.systemProperties["x-opt-offset"];
        } else {
          return "";
        }
      }
    },
    'properties': {
      value: null,
      writable: true
    },
    'sequenceNumber': {
      get: function () {
        if (this.systemProperties) {
          return this.systemProperties["x-opt-sequence-number"];
        } else {
          return 0;
        }
      }
    },
    'systemProperties': {
      value: message.messageAnnotations,
      writable: false
    },
    '_link': {
      value: link,
      writable: false
    },
    '_message': {
      value: message,
      writable: false
    }
  });
}

/**
 * Accept the message - only needed when you're using a custom policy that doesn't auto-settle messages. Tells server you have processed the message.
 */
EventData.prototype.accept = function() {
  this._link.accept(this._message);
};

/**
 * Reject the message - only needed when you're using a custom policy that doesn't auto-settle messages. Tells server you cannot process the message. Error must be a valid AMQP error, see README.
 */
EventData.prototype.reject = function(error) {
  this._link.reject(this._message, error);
};

/**
 * Release the message - only needed when you're using a custom policy that doesn't auto-settle messages. Tells server you did not processed the message and it can be redelivered.
 */
EventData.prototype.release = function() {
  this._link.release(this._message);
};

/**
 * Modify the message - only needed when you're using a custom policy that doesn't auto-settle messages. Tells server you want the message modified and redelivered.
 *
 * @param {Object}        [options] options used for a Modified outcome
 * @param {Boolean}       [options.deliveryFailed] count the transfer as an unsuccessful delivery attempt
 * @param {Boolean}       [options.undeliverableHere] prevent redelivery
 * @param {Object}        [options.messageAnnotations] message attributes to combine with existing annotations
 */
EventData.prototype.modify = function(options) {
  this._link.modify(this._message, options);
};

EventData.fromAmqpMessage = function (link, msg) {
  return new EventData(link, msg);
};

module.exports = EventData;