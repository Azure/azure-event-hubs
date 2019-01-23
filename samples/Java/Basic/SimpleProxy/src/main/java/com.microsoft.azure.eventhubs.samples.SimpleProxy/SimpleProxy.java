/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.SimpleProxy;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.microsoft.azure.eventhubs.*;

import java.io.IOException;
import java.net.*;
import java.nio.charset.Charset;
import java.time.Instant;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;


public class SimpleProxy {

    public static void main(String[] args)
            throws EventHubException, ExecutionException, InterruptedException, IOException {

        String proxyIpAddressStr = "---proxyhostname---";
        int proxyPort = 3128;

        //set the ProxySelector API; which offers the flexibility to select Proxy Server based on the Target URI.
        ProxySelector systemDefaultSelector = ProxySelector.getDefault();
        ProxySelector.setDefault(new ProxySelector() {
            @Override
            public List<Proxy> select(URI uri) {
                if (uri != null
                        && uri.getHost() != null
                        && uri.getHost().equalsIgnoreCase("youreventbushost.servicebus.windows.net")) {
                    LinkedList<Proxy> proxies = new LinkedList<>();
                    proxies.add(new Proxy(Proxy.Type.HTTP, new InetSocketAddress(proxyIpAddressStr, proxyPort)));
                    return proxies;
                }

                // preserve system default selector for the rest
                return systemDefaultSelector.select(uri);
            }
            @Override
            public void connectFailed(URI uri, SocketAddress sa, IOException ioe) {
                // trace and follow up on why proxy server is down
                if (uri == null || sa == null || ioe == null) {
                    throw new IllegalArgumentException("Arguments can't be null.");
                }

                systemDefaultSelector.connectFailed(uri, sa, ioe);

            }
        });

        // if the proxy being used, doesn't need any Authentication - "setting Authenticator" step may be omitted
        Authenticator.setDefault(new Authenticator() {
            @Override
            protected PasswordAuthentication getPasswordAuthentication() {
                if (this.getRequestorType() == RequestorType.PROXY
                        && this.getRequestingScheme().equalsIgnoreCase("basic")
                        && this.getRequestingHost().equals(proxyIpAddressStr)
                        && this.getRequestingPort() == proxyPort) {
                    return new PasswordAuthentication("userName", "password".toCharArray());
                }

                return super.getPasswordAuthentication();
            }
        });

        final ConnectionStringBuilder connStr = new ConnectionStringBuilder()
                .setNamespaceName("----NamespaceName-----") // to target National clouds - use .setEndpoint(URI)
                .setEventHubName("----EventHubName-----")
                .setSasKeyName("-----SharedAccessSignatureKeyName-----")
                .setSasKey("---SharedAccessSignatureKey---");

        connStr.setTransportType(TransportType.AMQP_WEB_SOCKETS);

        final ScheduledExecutorService executorService = Executors.newScheduledThreadPool(4);
        final EventHubClient ehClient = EventHubClient.createSync(connStr.toString(), executorService);
        final Gson gson = new GsonBuilder().create();
        PartitionSender sender = null;

        //sending events
        try {

            String payload = "Message " + Integer.toString(1);
            byte[] payloadBytes = gson.toJson(payload).getBytes(Charset.defaultCharset());
            EventData sendEvent = EventData.create(payloadBytes);

            sender = ehClient.createPartitionSenderSync("1");
            sender.sendSync(sendEvent);

            System.out.println(Instant.now() + ": Send Complete...");

        }
        finally {

        }

        final EventHubRuntimeInformation eventHubInfo = ehClient.getRuntimeInformation().get();
        final String partitionId = eventHubInfo.getPartitionIds()[1]; // get first partition's id

        final PartitionReceiver receiver = ehClient.createEpochReceiverSync(
                EventHubClient.DEFAULT_CONSUMER_GROUP_NAME,
                partitionId,
                EventPosition.fromEnqueuedTime(Instant.EPOCH),
                2345);

        try {
            Iterable<EventData> receivedEvents = receiver.receiveSync(10);

            if (receivedEvents != null) {
                for (final EventData receivedEvent : receivedEvents) {
                    if (receivedEvent.getBytes() != null)
                        System.out.println(String.format("Message Payload: %s", new String(receivedEvent.getBytes(), Charset.defaultCharset())));
                }
            }

        } finally {
            // cleaning up receivers is paramount;
            // Quota limitation on maximum number of concurrent receivers per consumergroup per partition is 5
            receiver.close()
                    .thenComposeAsync(aVoid -> ehClient.close(), executorService)
                    .whenCompleteAsync((t, u) -> {
                        if (u != null) {
                            // wire-up this error to diagnostics infrastructure
                            System.out.println(String.format("closing failed with error: %s", u.toString()));
                        }
                    }, executorService).get();

            executorService.shutdown();
        }
    }
}
