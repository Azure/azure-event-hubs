# Send events asynchronously to Azure Event Hubs using Java

To run the sample, you need to edit the [sample code](src/main/java/com/microsoft/azure/eventhubs/samples/asyncsend/AsyncSend.java) and provide the following information:

```java
    final String namespaceName = "----EventHubsNamespaceName-----";
    final String eventHubName = "----EventHubName-----";
    final String sasKeyName = "-----SharedAccessSignatureKeyName-----";
    final String sasKey = "---SharedAccessSignatureKey----";
```

## Prerequisites

Please refer to the [overview README](../../readme.md) for prerequisites and setting up the sample environment, including creating an Event Hubs cloud namespace and an Event Hub. 

## Build and run

The sample can be built independently with 

```bash
mvn clean package
```

and then run with (or just from VS Code or another Java IDE)

```bash
java -jar ./target/asyncsend-1.0.0-jar-with-dependencies.jar
```

## Understanding Async Send

This sample demonstrates using the asynchronous send() method on the EventHubClient class.

The corresponding sendSync() method is pretty simple: it waits for the the send operation to
be complete, then returns, indicating success, or throws, indicating an error. Unfortunately, it's
not fast, because it means waiting for the message to go across the network to the service, for
the service to store the message, and then for the result to come back across the network.

Switching to the asynchronous send() means your code must be more
sophisticated. By its nature, an asynchronous method just starts an operation and does not wait
for it to finish, so your code can be doing something useful while the operation proceeds in the
background. However, that means that the method itself cannot tell
you anything about the result of the operation, because there is no result yet when the method
returns. Java provides a standard class, CompletableFuture, for asynchronous methods to return,
which allows the calling code to obtain the result of the operation at a later time, and the Event
Hubs Java client makes use of that class. CompletableFuture offers some sophisticated abilities,
but this sample simply retrieves the result by calling get(), which will return when the operation
finishes (or has already finished) successfully, or throw an exception if there was an error.

One of the things your code can be doing while waiting for a send operation to finish is to prepare
and send additional messages, which this sample demonstrates. This is known as concurrent sending
and it is a good way to increase your throughput. The sample sends three groups of 100 messages each,
with one concurrent send, ten concurrent sends, and thirty concurrent sends respectively. It uses a
queue to save the CompletableFuture instances returned by send(), and when the queue grows larger
than the desired number of concurrent operations, it waits for the oldest CompletableFuture to
complete. One concurrent send is equivalent to making a synchronous call: each send must wait for the
previous one to finish. With ten concurrent sends, the average wait time and total execution time is
much smaller, because by the time the 11th send is attempted, the first send is mostly done, and the
same for the 12th send and the second, etc. With thirty concurrent sends, the average wait time is
nearly 0, because the first send is already finished by the time the 31st is attempted, etc. The
useful number of concurrent calls is limited by memory usage, CPU usage, network congestion, and other
factors, and we do not recommend trying more than about 100.

Error handling is also different when using asynchronous calls. Supposing the get() on the CompletableFuture
throws an error, indicating a failure, but you still want that message and would like to retry sending
it -- how do you know what message failed? This sample demonstrates one possible approach, by storing
the original message with the CompletableFuture. There are many possibilities and which is best
depends on the structure of your application.