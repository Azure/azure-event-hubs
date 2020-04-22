/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.AsyncSend;

import com.microsoft.azure.eventhubs.ConnectionStringBuilder;
import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.EventHubException;

import java.io.IOException;
import java.nio.charset.Charset;
import java.time.Instant;
import java.util.LinkedList;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

public class AsyncSend {

    public static void main(String[] args)
            throws EventHubException, ExecutionException, InterruptedException, IOException {

        final ConnectionStringBuilder connStr = new ConnectionStringBuilder()
                .setNamespaceName("Your Event Hubs namespace name") // to target National clouds - use .setEndpoint(URI)
                .setEventHubName("Your event hub")
                .setSasKeyName("Your policy name")
                .setSasKey("Your primary SAS key");

        // The Executor handles all asynchronous tasks and this is passed to the
        // EventHubClient instance. This enables the user to segregate their thread
        // pool based on the work load. This pool can then be shared across multiple
        // EventHubClient instances.
        final ScheduledExecutorService executorService = Executors.newScheduledThreadPool(4);

        // Each EventHubClient instance spins up a new TCP/SSL connection, which is
        // expensive. It is always a best practice to reuse these instances.
        final EventHubClient ehClient = EventHubClient.createFromConnectionStringSync(connStr.toString(), executorService);

        send100Messages(ehClient, 1);
        System.out.println();

        send100Messages(ehClient, 10);
        System.out.println();

        send100Messages(ehClient, 30);
        System.out.println();

        ehClient.closeSync();
        executorService.shutdown();
    }

    public static void send100Messages(final EventHubClient ehClient, final int inFlightMax) {
        System.out.println("Sending 100 messages with " + inFlightMax + " concurrent sends");

        LinkedList<MessageAndResult> inFlight = new LinkedList<MessageAndResult>();
        long totalWait = 0L;
        long totalStart = Instant.now().toEpochMilli();

        for (int i = 0; i < 100; i++) {
            String payload = "Message " + Integer.toString(i);
            byte[] payloadBytes = payload.getBytes(Charset.defaultCharset());
            MessageAndResult mar = new MessageAndResult(i, EventData.create(payloadBytes));
            long startWait = Instant.now().toEpochMilli();

            if (inFlight.size() >= inFlightMax) {
                MessageAndResult oldest = inFlight.remove();
                try {
                    oldest.result.get();
                    //System.out.println("Completed send of message " + oldest.messageNumber + ": succeeded");
                } catch (InterruptedException | ExecutionException e) {
                    System.out.println("Completed send of message " + oldest.messageNumber + ": failed: " + e.toString());
                }
            }

            long waitTime = Instant.now().toEpochMilli() - startWait;
            totalWait += waitTime;
            //System.out.println("Blocked time waiting to send (ms): " + waitTime);
            mar.result = ehClient.send(mar.message);
            //System.out.println("Started send of message " + mar.messageNumber);
            inFlight.add(mar);
        }

        System.out.println("All sends started");

        while (inFlight.size() > 0) {
            MessageAndResult oldest = inFlight.remove();
            try {
                oldest.result.get();
                //System.out.println("Completed send of message " + oldest.messageNumber + ": succeeded");
            } catch (InterruptedException | ExecutionException e) {
                System.out.println("Completed send of message " + oldest.messageNumber + ": failed: " + e.toString());
            }
        }

        System.out.println("All sends completed, average blocked time (ms): " + (totalWait / 100L));
        System.out.println("Total time to send 100 messages (ms): " + (Instant.now().toEpochMilli() - totalStart));
    }

    private static class MessageAndResult {
        public final int messageNumber;
        public final EventData message;
        public CompletableFuture<Void> result;

        public MessageAndResult(final int messageNumber, final EventData message) {
            this.messageNumber = messageNumber;
            this.message = message;
        }
    }
}
