/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.SimpleSend;

import com.azure.messaging.eventhubs.*;
import java.time.Instant;

public class SimpleSend {
    private static final String connectionString = "<EVENT HUBS NAMESPACE - CONNECTION STRING>";
    private static final String eventHubName = "<EVENT HUB NAME>";

    public static void main(String[] args) {
        publishEvents();
    }

    /**
     * Code sample for publishing events.
     * @throws IllegalArgumentException if the EventData is bigger than the max batch size.
     */
    public static void publishEvents() {
        // create a producer client
        EventHubProducerClient producer = new EventHubClientBuilder()
            .connectionString(connectionString, eventHubName)
            .buildProducerClient();

        try {
            // Start preparting a batch of events
            EventDataBatch eventDataBatch = producer.createBatch();
            producer.send(eventDataBatch);                

            for (int i = 0; i < 5; i++) {

                // prepare data for the event
                EventData eventData = new EventData("Message " + Integer.toString(i));
                // add the event to the batch
                eventDataBatch.tryAdd(eventData);
                // send event to the event hub
                producer.send(eventDataBatch);                
            }

            System.out.println(Instant.now() + ": Send Complete...");
        } finally {
            producer.close();
        }     
    }
}

