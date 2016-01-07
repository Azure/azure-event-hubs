event-processor-host
================

[![npm version](https://badge.fury.io/js/event-processor-host.svg)](http://badge.fury.io/js/event-processor-host)

## Usage ##

This library is primarily event-based with promises for starting/stopping (for now, using [Bluebird](http://bluebirdjs.com/docs/getting-started.html)). See `tests/event_processor_host_test.js` for a basic example. 

The simplest usage is to instantiate the main `EventProcessorHost` class with a storage details and an `EventHubClient`, or use the static factory method `EventProcessorHost.fromConnectionString(name, consumerGroup, storageConnectionString, eventHubConnectionString, eventHubPath)`. Once you have a host, you call attach event handlers to the exposed events (`EventProcessorHost.Opened`, `.Closed`, `.Message`) and then `start` with an optional partition filter and wait for it to start up. The optional filter allows you to only receive from certain partitions, which in the case of very large event hubs could be useful (e.g. for a 1024-partition hub, you could receive from 128 partitions on each of 8 host groups). 
 
## Example - Basic life-cycle, listening on all partitions. ##

```js
var Promise = require('bluebird');
var EventProcessorHost = require('event-processor-host').EventProcessorHost;

var ephost = EventProcessorHost.fromConnectionString('eph', '$Default', 'DefaultEndpointsProtocol=https;AccountName=name;AccountKey=key', 'Endpoint=sb://my-servicebus-namespace.servicebus.windows.net/;SharedAccessKeyName=my-SA-name;SharedAccessKey=my-SA-key', 'myeventhub');
ephost.on(EventProcessorHost.Message, function (context, message) {
    console.log('Receved a message on partition ' + context.partitionId);
    console.log(message);
});
ephost.start().then(function() {
    return Promise.delay(1000);
}).then(function() {
    return ephost.stop();
});
```

