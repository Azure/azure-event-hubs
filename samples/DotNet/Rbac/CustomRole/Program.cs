// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace CustomRole
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;
    using Microsoft.Azure.EventHubs.Processor;
    using Microsoft.Identity.Client;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Auth;

    public class Program
    {
        private const string Authority = "Authority of tenant";
        private const string ClientId = "AAD app id";
        private const string ClientSecret = "AAD app secret";
        private const string EventHubNamespace = "FQDN of Event Hubs namespace";
        private const string EventHubName = "Event hub name";
        private const string StorageAccountName = "Storage account name";
        private const string StorageContainerName = "Storage account container name";

        static IConfidentialClientApplication tokenClient;

        public static async Task Main(string[] args)
        {
            // Use the same identity client for both Event Hubs and Storage
            tokenClient = ConfidentialClientApplicationBuilder.Create(ClientId)
                       .WithAuthority(Authority)
                       .WithClientSecret(ClientSecret)
                       .Build();

            // Create Storage client with access token provider
            var tokenAndFrequency = await TokenRenewerAsync(tokenClient, CancellationToken.None);
            var tokenCredential = new TokenCredential(tokenAndFrequency.Token,
                                                        TokenRenewerAsync,
                                                        tokenClient,
                                                        tokenAndFrequency.Frequency.Value);
            var storageCredentials = new StorageCredentials(tokenCredential);
            CloudStorageAccount cloudStorageAccount = new CloudStorageAccount(storageCredentials, StorageAccountName, string.Empty, true);

            // Create Event Hubs account with access token provider
            TokenProvider tp = TokenProvider.CreateAzureActiveDirectoryTokenProvider(
                new AzureActiveDirectoryTokenProvider.AuthenticationCallback(GetAccessToken), Authority, tokenClient);

            Console.WriteLine("Registering EventProcessor...");
            var eventProcessorHost = new EventProcessorHost(
                new Uri(EventHubNamespace),
                EventHubName,
                PartitionReceiver.DefaultConsumerGroupName,
                tp,
                cloudStorageAccount,
                StorageContainerName);

            // Registers the Event Processor Host and starts receiving messages
            await eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();

            Console.WriteLine("Receiving. Press enter key to stop worker.");
            Console.ReadLine();

            // Disposes of the Event Processor Host
            await eventProcessorHost.UnregisterEventProcessorAsync();
        }

        private static async Task<NewTokenAndFrequency> TokenRenewerAsync(Object state, CancellationToken cancellationToken)
        {
            var authResult = await ((IConfidentialClientApplication)state)
                .AcquireTokenForClient(new string[] { $"https://storage.azure.com/.default" }).ExecuteAsync();

            // Renew the token 5 minutes before it expires.
            var next = (authResult.ExpiresOn - DateTimeOffset.UtcNow) - TimeSpan.FromMinutes(5);
            if (next.Ticks < 0)
            {
                next = default(TimeSpan);
                Console.WriteLine("Renewing token...");
            }

            // Return the new token and the next refresh time.
            return new NewTokenAndFrequency(authResult.AccessToken, next);
        }

        static async Task<string> GetAccessToken(string audience, string authority, object state)
        {
            var authResult = await tokenClient
                .AcquireTokenForClient(new string[] { $"{audience}/.default" }).ExecuteAsync();
            return authResult.AccessToken;
        }
    }
}
