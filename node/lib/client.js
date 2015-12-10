// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var Promise = require('bluebird');
var ArgumentError = require('azure-iot-common').errors.ArgumentError;
var makeConfig = require('./config.js');

function EventHubClient(config) {
  if (!config.host) throw new ArgumentError('Argument config is missing property host');
}

EventHubClient.fromConnectionString = function (connectionString, path) {
  if (!connectionString) {
    throw new ArgumentError('Missing argument connectionString');
  }
  
  var config = makeConfig(connectionString, path);
  if (!config.path) {
    throw new ArgumentError('Connection string doesn\'t have EntityPath, or missing argument path');
  }

  return new EventHubClient(config);
};

EventHubClient.prototype.open = function () {
  return Promise.resolve();
};

module.exports = EventHubClient;