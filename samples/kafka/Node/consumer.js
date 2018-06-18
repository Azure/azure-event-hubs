var Transform = require('stream').Transform;
var Kafka = require('node-rdkafka');

var stream = Kafka.KafkaConsumer.createReadStream({
    'metadata.broker.list': 'EVENTHUB_FQDN',
    'group.id': 'EVENTHUB_CONSUMER_GROUP', //The default for EvenHubs is $Default
    'socket.keepalive.enable': true,
    'enable.auto.commit': false,
    'security.protocol': 'SASL_SSL',
    'sasl.mechanisms': 'PLAIN',
    'sasl.username': '$ConnectionString',
    'sasl.password': 'EVENTHUB_CONNECTION_STRING'

}, {}, {
        topics: 'test',
        waitInterval: 0,
        objectMode: false
    });

stream.on('error', function (err) {
    if (err) console.log(err);
    process.exit(1);
});

stream
    .pipe(process.stdout);

stream.on('error', function (err) {
    console.log(err);
    process.exit(1);
});

stream.consumer.on('event.error', function (err) {
    console.log(err);
})