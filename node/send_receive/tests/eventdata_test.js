// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var EventData = require('../lib/eventdata.js');
var chai = require('chai');
chai.should();

var testAnnotations = {
  "x-opt-enqueued-time": Date.now(),
  "x-opt-offset": "42",
  "x-opt-sequence-number": 1337,
  "x-opt-partition-key": 'key'
};

var testBody = '{ "foo": "bar" }';

var testMessage = {
  body: testBody,
  messageAnnotations: testAnnotations
};

var link = {};

describe('EventData', function(){
  describe('.fromAmqpMessage', function () {
    it('returns an instance of EventData', function() {
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      testEventData.should.be.instanceOf(EventData);
    });

    it('populates systemProperties with the message properties', function () {
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      console.log('Test: ', JSON.stringify(testEventData));
      testEventData.systemProperties.should.equal(testAnnotations);
    });

    it('populates body with the message body', function () {
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      testEventData.body.should.equal(testBody);
    });
  });

  describe('#properties', function() {
    it('enqueuedTimeUtc gets the enqueued time from system properties', function(){
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      testEventData.enqueuedTimeUtc.should.equal(testAnnotations['x-opt-enqueued-time']);
    });

    it('offset gets the offset from system properties', function(){
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      testEventData.offset.should.equal(testAnnotations['x-opt-offset']);
    });

    it('sequenceNumber gets the sequence number from system properties', function(){
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      testEventData.sequenceNumber.should.equal(testAnnotations['x-opt-sequence-number']);
    });

    it('partitionKey gets the sequence number from system properties', function(){
      var testEventData = EventData.fromAmqpMessage(link, testMessage);
      testEventData.partitionKey.should.equal(testAnnotations['x-opt-partition-key']);
    });

    [null, undefined].forEach(function(systemProp) {
      it('enqueuedTimeUtc returns \'null\' if systemProperties are falsy', function(){
        var testEventData = new EventData(link, testBody, systemProp);
        chai.expect(testEventData.enqueuedTimeUtc).to.equal(null);
      });

      it('offset returns \'\' if systemProperties are falsy', function(){
        var testEventData = new EventData(link, testBody, systemProp);
        testEventData.offset.should.equal("");
      });

      it('sequenceNumber returns \'0\' if systemProperties are falsy', function(){
        var testEventData = new EventData(link, testBody, systemProp);
        testEventData.sequenceNumber.should.equal(0);
      });

      it('partitionKey returns \'null\' if systemProperties are falsy', function(){
        var testEventData = new EventData(link, testBody, systemProp);
        chai.expect(testEventData.partitionKey).to.equal(null);
      });
    });
  });
});