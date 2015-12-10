// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var chai = require('chai');
var chaiAsPromised = require('chai-as-promised');

chai.should();
chai.use(chaiAsPromised);

var EventHubClient = require('../lib/client.js');

describe('EventHubClient', function () {
  var client;

  beforeEach('create the client', function () {
    client = EventHubClient.fromConnectionString(process.env.EVENTHUB_CONNECTION_STRING, process.env.EVENTHUB_PATH);
  });

  afterEach('close the connection', function () {
    return client.close();
  });

  this.timeout(15000);

  describe('#open', function () {
    it('opens a connection to the Event Hub', function () {
      return client.open().should.be.fulfilled;
    });
  });
  
  describe('#close', function () {
    it('is a no-op when the connection is already closed', function () {
      return client.close().should.be.fulfilled;
    });
    
    it('closes an open connection', function () {
      return client.open()
        .then(function () {
          return client.close().should.be.fulfilled;
        });
    });
  });
});