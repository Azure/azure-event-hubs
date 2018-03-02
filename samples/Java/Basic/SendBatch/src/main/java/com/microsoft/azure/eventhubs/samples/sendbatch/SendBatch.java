/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.sendbatch;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.microsoft.azure.eventhubs.ConnectionStringBuilder;
import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventDataBatch;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.EventHubException;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Random;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class SendBatch {

    public static void main(String[] args)
            throws EventHubException, IOException {

        final ConnectionStringBuilder connStr = new ConnectionStringBuilder()
                .setNamespaceName("----ServiceBusNamespaceName-----") // to target National clouds - use .setEndpoint(URI)
                .setEventHubName("----EventHubName-----")
                .setSasKeyName("-----SharedAccessSignatureKeyName-----")
                .setSasKey("---SharedAccessSignatureKey----");

        final Gson gson = new GsonBuilder().create();
        final ExecutorService executorService = Executors.newSingleThreadExecutor();
        final EventHubClient sender = EventHubClient.createSync(connStr.toString(), executorService);

        try {
            for (int batchNumber = 0; batchNumber < 10; batchNumber++) {

                final EventDataBatch events = sender.createBatch();
                EventData sendEvent;

                // This do..while loop demonstrates - Maximizing batch size for every send call.
                // sending multiple EventData's in one batch - guarantees order among the events sent in this batch
                // and provides transactional semantics for the batch (all-or-none)
                do {
                    final PayloadEvent payload = new PayloadEvent(100 + batchNumber);
                    final byte[] payloadBytes = gson.toJson(payload).getBytes(Charset.defaultCharset());

                    sendEvent = EventData.create(payloadBytes);
                    sendEvent.getProperties().put("from", "javaClient");
                } while(events.tryAdd(sendEvent));

                sender.sendSync(events);
                System.out.println(String.format("Sent Batch - Batch Id: %s, Size: %s", batchNumber, events.getSize()));
            }
        } finally {
            sender.closeSync();
            executorService.shutdown();
        }
    }

    /**
     * actual application-payload, ex: a telemetry event
     */
    static final class PayloadEvent {
        PayloadEvent(final int seed) {
            this.id = "telemetryEvent1-critical-eventid-2345" + seed;
            this.strProperty = "I am a mock telemetry event from JavaClient.";
            this.longProperty = seed * new Random().nextInt(seed);
            this.intProperty = seed * new Random().nextInt(seed);
        }

        final public String id;
        final public String strProperty;
        final public long longProperty;
        final public int intProperty;
    }

}
