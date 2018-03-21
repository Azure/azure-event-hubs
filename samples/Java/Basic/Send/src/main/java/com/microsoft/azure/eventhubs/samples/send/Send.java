/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.send;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.microsoft.azure.eventhubs.ConnectionStringBuilder;
import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.PartitionSender;
import com.microsoft.azure.eventhubs.EventHubException;

import java.io.IOException;
import java.nio.charset.Charset;
import java.time.Instant;
import java.util.Random;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;

public class Send {

    public static void main(String[] args)
            throws EventHubException, ExecutionException, InterruptedException, IOException {
    	
    	final ConnectionStringBuilder connStr = new ConnectionStringBuilder()
                .setNamespaceName("----NamespaceName-----")// to target National clouds - use .setEndpoint(URI)
                .setEventHubName("----EventHubName-----")
                .setSasKeyName("-----SharedAccessSignatureKeyName-----")
                .setSasKey("---SharedAccessSignatureKey---"); 

        final Gson gson = new GsonBuilder().create();

        final PayloadEvent payload = new PayloadEvent(1);
        byte[] payloadBytes = gson.toJson(payload).getBytes(Charset.defaultCharset());
        final EventData sendEvent = EventData.create(payloadBytes);

        final ExecutorService executorService = Executors.newSingleThreadExecutor();
        final EventHubClient ehClient = EventHubClient.createSync(connStr.toString(), executorService);;
        PartitionSender sender = null;

        try {
            // senders
            // Type-1 - Send - not tied to any partition
            // EventHubs service will round-robin the events across all EventHubs partitions.
            // This is the recommended & most reliable way to send to EventHubs.
            ehClient.send(sendEvent).get();

            // Partition-sticky Sends
            // Type-2 - Send using PartitionKey - all Events with Same partitionKey will land on the Same Partition
            final String partitionKey = "partitionTheStream";
            ehClient.sendSync(sendEvent, partitionKey);

            // Type-3 - Send to a Specific Partition
            sender = ehClient.createPartitionSenderSync("0");
            sender.sendSync(sendEvent);

            System.out.println(Instant.now() + ": Send Complete...");
            System.in.read();
        } finally {
            if (sender != null) {
                sender.close()
                        .thenComposeAsync(aVoid -> ehClient.close(), executorService)
                        .whenCompleteAsync((aVoid1, throwable) -> {
                            if (throwable != null) {
                                // wire-up this error to diagnostics infrastructure
                                System.out.println(String.format("closing failed with error: %s", throwable.toString()));
                            }
                        }, executorService).get();
            } else {
                ehClient.closeSync();
            }

            executorService.shutdown();
        }
    }

    /**
     * actual application-payload, ex: a telemetry event
     */
    static final class PayloadEvent {
        PayloadEvent(final int seed) {
            this.id = "telemetryEvent1-critical-eventid-2345" + seed;
            this.strProperty = "This is a sample payloadEvent, which could be wrapped using eventdata and sent to eventhub." +
                    " None of the payload event properties will be looked-at by EventHubs client or Service." +
                    " As far as EventHubs service/client is concerted, it is plain bytes being sent as 1 Event.";
            this.longProperty = seed * new Random().nextInt(seed);
            this.intProperty = seed * new Random().nextInt(seed);
        }

        public String id;
        public String strProperty;
        public long longProperty;
        public int intProperty;
    }
}
