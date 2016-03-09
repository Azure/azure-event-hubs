azure-event-hubs
================

[![npm version](https://badge.fury.io/js/azure-event-hubs.svg)](http://badge.fury.io/js/azure-event-hubs)

## Usage ##

This library is primarily promise-based (for now, using [Bluebird](http://bluebirdjs.com/docs/getting-started.html)). See `tests/sender_test.js` or `tests/receiver_test.js` for some basic examples. 

The simplest usage is to instantiate the main `EventHubClient` class with a `ConnectionConfig`, or use the static factory method `EventHubClient.fromConnectionString(_connection-string_, _event-hub-path_)`. Once you have a client, you can use it to create senders and/or receivers using the `client.createSender` or `client.createReceiver` methods. Receivers emit `message` events when new messages come in, while senders have a simple `send` method that allows you to easily send messages (with an optional partition key). 
 
## Example 1 - Get the partition IDs. ##

```js
var EventHubClient = require('azure-event-hubs').Client;

var client = EventHubClient.fromConnectionString('Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 'myeventhub')
client.getPartitionIds().then(function(ids) {
    ids.forEach(function(id) { console.log(id); });
});
```

## Example 2 - Create a receiver ##

Creates a receiver on partition ID 10, for messages that come in after "now".

```js
var EventHubClient = require('azure-event-hubs').Client;

var client = EventHubClient.fromConnectionString('Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 'myeventhub')
client.createReceiver('$Default', '10', { startAfterTime: Date.now() })
    .then(function (rx) {
        rx.on('errorReceived', function (err) { console.log(err); }); 
        rx.on('message', function (message) {
            var body = message.body;
            // @see eventdata.js
            var enqueuedTime = Date.parse(message.enqueuedTimeUtc);
        });
    });

```

## Example 3 - Create a sender v1 ##

Creates a sender, sends to a given partition "key" which is then hashed to a partition ID (so all messages with the same key will go to the same ID, but load is balanced between partitions). 

```js
var EventHubClient = require('azure-event-hubs').Client;

var client = EventHubClient.fromConnectionString('Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 'myeventhub')
client.createSender()
    .then(function (tx) {
        tx.on('errorReceived', function (err) { console.log(err); });
        tx.send({ contents: 'Here is some text sent to partition key my-pk.' }, 'my-pk'); 
    });
```

## Example 4 - Create a sender v2 ##

Creates a sender against a given partition ID (10). You _should_ use send to a given partition _key_, but if you _need_ to send to a given partition ID we'll let you do it. 

```js
var EventHubClient = require('azure-event-hubs').Client;

var client = EventHubClient.fromConnectionString('Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 'myeventhub')
client.createSender('10')
    .then(function (tx) {
        tx.on('errorReceived', function (err) { console.log(err); });
        tx.send({ contents: 'Here is some text sent to partition 10.' }); 
    });
```

## Example 5 - Create a receiver with a custom policy ##

Creates a receiver for a given partition ID, with a larger max message size than the default (10000 bytes).

```js
var EventHubClient = require('azure-event-hubs').Client;

var client = EventHubClient.fromConnectionString(
    'Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 
    'myeventhub',
    { receiverLink: { attach: { maxMessageSize: 100000 }}});
client.createReceiver('$Default', '10', { startAfterTime: Date.now() })
    .then(function (rx) {
        rx.on('errorReceived', function (err) { console.log(err); }); 
        rx.on('message', function (message) {
            var body = message.body;
            var enqueuedTime = Date.parse(message.enqueuedTimeUtc);
        });
    });

```

## Example 6 - Create a throttled receiver ##

Creates a receiver for a given partition ID, with a custom policy that throttles messages. Initially the client can deliver 10 messages, and after the "credit" goes below 5, the receiver will start allowing more messages only for those that have been "settled". Settling can be done via the settlement methods on EventData, such as `accept` or `reject`.
 
```js
var EventHubClient = require('azure-event-hubs').Client;
var amqp10 = require('amqp10');

// Custom AMQP transport factory with the policy we want.
var transportFactory = function() { return new amqp10.Client(amqp10.Policy.Utils.RenewOnSettle(10, 5, Policy.EventHub)); };

var client = EventHubClient.fromConnectionString(
    'Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 
    'myeventhub',
    transportFactory);
client.createReceiver('$Default', '10', { startAfterTime: Date.now() })
    .then(function (rx) {
        rx.on('errorReceived', function (err) { console.log(err); }); 
        rx.on('message', function (message) {
            var body = message.body;
            var enqueuedTime = Date.parse(message.enqueuedTimeUtc);
            // ... do some processing ...
            message.accept();
        });
    });


```