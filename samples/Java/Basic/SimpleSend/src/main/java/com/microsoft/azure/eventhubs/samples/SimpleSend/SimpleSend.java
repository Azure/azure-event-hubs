/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.eventhubs.samples.SimpleSend;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.microsoft.azure.eventhubs.ConnectionStringBuilder;
import com.microsoft.azure.eventhubs.EventData;
import com.microsoft.azure.eventhubs.EventHubClient;
import com.microsoft.azure.eventhubs.EventHubException;

import java.io.IOException;
import java.net.URISyntaxException;
import java.nio.charset.Charset;
import java.time.Instant;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

public class SimpleSend {

    final java.net.URI namespace = new java.net.URI("YourEventHubsNamespace.servicebus.windows.net");
    final String eventhub = "Your event hub";
    final String authority = "https://login.windows.net/replaceWithTenantIdGuid";
    final String clientId = "replaceWithClientIdGuid";
    final String clientSecret = "replaceWithClientSecret";

    public SimpleSend() throws URISyntaxException {
    }

    public static int main(String[] args)
            throws EventHubException, ExecutionException, InterruptedException, IOException, URISyntaxException {

        SimpleSend ss = new SimpleSend();
        return ss.run(args);
    }

    private int run(String[] args) throws IOException {

        System.out.println("Choose an action:");
        System.out.println("[A] Authenticate via Managed Identity and send / receive.");
        System.out.println("[B] Authenticate via interactive logon and send / receive.");
        System.out.println("[C] Authenticate via client secret and send / receive.");
        System.out.println("[D] Authenticate via certificate and send / receive.");

        char key = (char)System.in.read();
        char keyPressed = Character.toUpperCase(key);

        try {
            switch (keyPressed) {
                case 'A':
                    managedIdentityScenario(); // Use managed identity, either user-assigned or system-assigned.
                    break;
                case 'B':
                    userInteractiveLoginScenario(); // Provision a native app. Make sure to give microsoft.eventhubs under required permissions
                    break;
                case 'C':
                    clientCredentialsScenario(); // This scenario needs app registration in AAD and IAM registration. Only web api will work in AAD app registration.
                    break;
                case 'D':
                    clientAssertionCertScenario();
                    break;
                default:
                    System.out.println("Unknown command, press enter to exit");
                    System.in.read();
                    return -1;
            }
        }
        catch (Exception ex) {
            System.out.println("Error during execution. Exception: " + ex.toString());
            return -1;
        }

        return 0;
    }

    private ScheduledExecutorService getScheduledExecutorService() {
        // The Executor handles all asynchronous tasks and this is passed to the EventHubClient instance.
        // This enables the user to segregate their thread pool based on the work load.
        // This pool can then be shared across multiple EventHubClient instances.
        // The following sample uses a single thread executor, as there is only one EventHubClient instance,
        // handling different flavors of ingestion to Event Hubs here.
        final ScheduledExecutorService executorService = Executors.newScheduledThreadPool(4);
        return executorService;
    }

    private void managedIdentityScenario() throws IOException {

        final ConnectionStringBuilder connStr = new ConnectionStringBuilder()
                .setEndpoint(this.namespace)
                .setEventHubName(this.eventhub)
                .setAuthentication(ConnectionStringBuilder.MANAGED_IDENTITY_AUTHENTICATION);
        ScheduledExecutorService executorService = getScheduledExecutorService();

        final EventHubClient ehClient = EventHubClient.createSync(connStr.toString(), executorService);

        sendReceive(ehClient, executorService);
    }

    private void userInteractiveLoginScenario() throws IOException {

        final AuthCallback callback = new AuthCallback(clientId, clientSecret);
        ScheduledExecutorService executorService = getScheduledExecutorService();

        final EventHubClient ehClient = EventHubClient.createWithAzureActiveDirectory(namespace, eventhub, callback, authority, executorService, null).get();

        sendReceive(ehClient, executorService);
    }

    private void clientCredentialsScenario() throws IOException {

        final AuthCallback callback = new AuthCallback(clientId, clientSecret);
        ScheduledExecutorService executorService = getScheduledExecutorService();

        final AzureActiveDirectoryTokenProvider aadTokenProvider = new AzureActiveDirectoryTokenProvider(callback, authority, null);

        final EventHubClient ehClient = EventHubClient.createWithTokenProvider(namespace, eventhub, aadTokenProvider, executorService, null).get();

        sendReceive(ehClient, executorService);
    }

    private void clientAssertionCertScenario() throws IOException {

        final CustomTokenProvider tokenProvider = new CustomTokenProvider(authority, clientId, clientSecret);
        ScheduledExecutorService executorService = getScheduledExecutorService();

        final EventHubClient ehClient = EventHubClient.createWithTokenProvider(namespace, eventhub, tokenProvider, executorService, null).get();

        sendReceive(ehClient, executorService);
    }

    private void sendReceive(EventHubClient ehClient, ScheduledExecutorService executorService) throws IOException {
        try {
            final Gson gson = new GsonBuilder().create();

            for (int i = 0; i < 100; i++) {

                String payload = "Message " + Integer.toString(i);
                //PayloadEvent payload = new PayloadEvent(i);
                byte[] payloadBytes = gson.toJson(payload).getBytes(Charset.defaultCharset());
                EventData sendEvent = EventData.create(payloadBytes);

                // Send - not tied to any partition
                // Event Hubs service will round-robin the events across all Event Hubs partitions.
                // This is the recommended & most reliable way to send to Event Hubs.
                ehClient.sendSync(sendEvent);
            }

            System.out.println(Instant.now() + ": Send Complete...");
            System.out.println("Press Enter to stop.");
            System.in.read();
        } finally {
            ehClient.closeSync();
            executorService.shutdown();
        }
    }
}
