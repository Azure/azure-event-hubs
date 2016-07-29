/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.servicebus.ConnectionStringBuilder;

import java.util.concurrent.BlockingQueue;

/**
 * This code sample is apart of the TwitterProducerSample - if you have seen TwitterProducerSample.java, I would
 * start there.
 *
 * In the threads created in the mentioned java file, we are simply take messages from our message queue and sending
 * them to our EventHub continuously.
 */
public class TweetHandler implements Runnable {
    private Thread t;
    private BlockingQueue<String> msgQueue;

    TweetHandler(BlockingQueue<String> mq) { msgQueue = mq; }

    public void run() {
        EventHubClient ehClient = null;
        try {
            final String namespaceName = "NAMESPACE_NAME";
            final String eventHubName = "EVENTHUB_NAME";
            final String sasKeyName = "SAS_KEY_NAME";
            final String sasKey = "SAS_KEY";
            ConnectionStringBuilder connStr = new ConnectionStringBuilder(namespaceName, eventHubName, sasKeyName, sasKey);
            ehClient = EventHubClient.createFromConnectionString(connStr.toString()).get();
        } catch (Exception up) {
            System.out.println(up.getMessage());
        }

        // Continuously send tweets to EventHub
        while (true) {
            try {
                String msg = msgQueue.take();
                byte[] payloadBytes = msg.getBytes();
                EventData sendEvent = new EventData(payloadBytes);
                ehClient.send(sendEvent).whenCompleteAsync((aVoid, e) -> { if (e != null) { System.out.println("e wasn't null.."); }});
                System.out.println(String.format("Sent event..."));
            } catch (Exception e) {
                System.out.println(e.getMessage());
            }
        }
    }

    public void start() {
        if (t == null) {
            System.out.println("Starting...");
            t = new Thread (this);
            t.start();
        }
    }
}
