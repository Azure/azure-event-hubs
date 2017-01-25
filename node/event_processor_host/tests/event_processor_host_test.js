// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var chai = require('chai');
var chaiAsPromised = require('chai-as-promised');

var should = chai.should();
chai.use(chaiAsPromised);

var uuid = require('uuid');
var Promise = require('bluebird');
var ArgumentError = require('azure-iot-common').errors.ArgumentError;
var EventHubClient = require('azure-event-hubs').Client;
var EventProcessorHost = require('../lib/event_processor_host');

describe('EventProcessorHost', function () {
  describe('#constructor', function () {
    it('throws if arguments are null', function() {
      (function () { return new EventProcessorHost(null, 'cg', 'scs', 'ehc'); }).should.throw(ArgumentError, 'name');
      (function () { return new EventProcessorHost('n', null, 'scs', 'ehc'); }).should.throw(ArgumentError, 'consumerGroup');
      (function () { return new EventProcessorHost('n', 'cg', null, 'ehc'); }).should.throw(ArgumentError, 'storageConnectionString');
      (function () { return new EventProcessorHost('n', 'cg', 'scs', null); }).should.throw(ArgumentError, 'eventHubClient');
    });
  });
});

before('validate environment', function () {
  should.exist(process.env.STORAGE_CONNECTION_STRING,
    'define STORAGE_CONNECTION_STRING in your environment before running integration tests.');
  should.exist(process.env.EVENTHUB_CONNECTION_STRING,
    'define EVENTHUB_CONNECTION_STRING in your environment before running integration tests.');
  should.exist(process.env.EVENTHUB_PATH,
    'define EVENTHUB_PATH in your environment before running integration tests.');
});

describe('EventProcessorHost', function () {
  var host;
  var name = 'fromnode';

  beforeEach('create the event processor host', function () {
    host = EventProcessorHost.fromConnectionString(name, '$Default', process.env.STORAGE_CONNECTION_STRING, process.env.EVENTHUB_CONNECTION_STRING, process.env.EVENTHUB_PATH);
  });

  afterEach('stop the event processor host', function () {
    return host.stop();
  });

  // Needs a long timeout since it's grabbing 16 leases and setting up 16 receivers.
  //this.timeout(60000);

  describe('#start', function () {
    it('starts an Event Processor Host', function () {
      return host.start().should.be.fulfilled;
    });

    it('starts receivers when it gets leases', function(done) {
      var partitions = {};
      var ehc = EventHubClient.fromConnectionString(process.env.EVENTHUB_CONNECTION_STRING, process.env.EVENTHUB_PATH);
      ehc.getPartitionIds()
        .then(function (ids) {
          ids.forEach(function (id) {
            partitions[id] = false;
          });
          host.on(EventProcessorHost.Opened, function(ctx) {
            partitions[ctx.partitionId] = true;
            var allSet = true;
            for (var p in partitions) {
              if (!partitions.hasOwnProperty(p)) continue;
              if (!partitions[p]) allSet = false;
            }
            if (allSet) {
              done();
            }
          });
          return host.start();
        });
    });

    it('sets context when it gets messages', function(done) {
      var partitions = {};
      var msgId = uuid.v4();
      var ehc = EventHubClient.fromConnectionString(process.env.EVENTHUB_CONNECTION_STRING, process.env.EVENTHUB_PATH);
      Promise.join(ehc.getPartitionIds(), ehc.createSender(),
        function (ids, sender) {
          ids.forEach(function (id) {
            partitions[id] = false;
          });
          host.on(EventProcessorHost.Message, function(ctx, m) {
            console.log('Rx message from ' + ctx.partitionId + ': ' + JSON.stringify(m));
            if (m.body.id === msgId) {
              ctx.checkpoint().then(function() {
                return ctx.lease.getContents();
              }).then(function (contents){
                console.log('Seen expected message. New lease contents: ' + contents);
                var parsed = JSON.parse(contents);
                parsed.Offset.should.eql(m.annotations.value['x-opt-offset']);
                done();
              });
            }
          });
          host.on(EventProcessorHost.Opened, function(ctx) {
            partitions[ctx.partitionId] = true;
            var allSet = true;
            for (var p in partitions) {
              if (!partitions.hasOwnProperty(p)) continue;
              if (!partitions[p]) allSet = false;
            }
            if (allSet) {
              sender.send({ message: 'Testing', id: msgId });
            }
          });
          return host.start();
        });
    });
  });
  
  describe('#stop', function () {
    it('is a no-op when the EPH has not been started', function () {
      return host.stop().should.be.fulfilled;
    });
    
    it('stops a started EPH', function () {
      return host.start()
        .then(function () {
          return host.stop().should.be.fulfilled;
        });
    });
  });
});