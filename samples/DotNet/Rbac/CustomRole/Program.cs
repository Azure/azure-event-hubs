// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace CustomRole
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.EventHubs;
    using Microsoft.Azure.EventHubs.Processor;
    using Microsoft.Azure.Storage;
    using Microsoft.Azure.Storage.Auth;
    using Microsoft.Identity.Client;

    public class Program
    {
        static readonly string TenantId = "";
        static readonly string ClientId = "";
        static readonly string ClientSecret = "";
        static readonly string EventHubNamespace = "";
        static readonly string EventHubName = "";
        static readonly string StorageContainerName = "";
        
        public static void Main(string[] args)
        {
            MainAsync(args).GetAwaiter().GetResult();
        }

        private static async Task MainAsync(string[] args)
        {
            // Create Azure Storage Client with token provider.
            var tokenAndFrequency = await StorageTokenRenewerAsync(null, CancellationToken.None);
            var tokenCredential = new TokenCredential(tokenAndFrequency.Token, StorageTokenRenewerAsync, null, tokenAndFrequency.Frequency.Value);
            var storageCredentials = new StorageCredentials(tokenCredential);
            var cloudStorageAccount = new CloudStorageAccount(storageCredentials, false);

            // Create Azure Active Directory token provider for Event Hubs access.
            TokenProvider tokenProvider = TokenProvider.CreateAzureActiveDirectoryTokenProvider(
                async (audience, authority, state) =>
                {
                    IConfidentialClientApplication app = ConfidentialClientApplicationBuilder.Create(ClientId)
                               .WithTenantId(TenantId)
                               .WithClientSecret(ClientSecret)
                               .Build();

                    var authResult = await app.AcquireTokenForClient(new string[] { $"{audience}/.default" }).ExecuteAsync();
                    return authResult.AccessToken;
                });

            Console.WriteLine("Registering EventProcessor...");

            var eventProcessorHost = new EventProcessorHost(
                new Uri(EventHubNamespace),
                EventHubName,
                PartitionReceiver.DefaultConsumerGroupName,
                tokenProvider,
                null,
                StorageContainerName);

            // Registers the Event Processor Host and starts receiving messages
            await eventProcessorHost.RegisterEventProcessorAsync<SimpleEventProcessor>();

            Console.WriteLine("Receiving. Press enter key to stop worker.");
            Console.ReadLine();

            // Disposes of the Event Processor Host
            await eventProcessorHost.UnregisterEventProcessorAsync();
        }

        static async Task<NewTokenAndFrequency> StorageTokenRenewerAsync(object state, CancellationToken cancellationToken)
        {
            // Specify the resource ID for requesting Azure AD tokens for Azure Storage.
            const string StorageResource = "https://storage.azure.com/";

            IConfidentialClientApplication app = ConfidentialClientApplicationBuilder.Create(ClientId)
                       .WithTenantId(TenantId)
                       .WithClientSecret(ClientSecret)
                       .Build();

            var authResult = await app.AcquireTokenForClient(new string[] { $"{StorageResource}/.default" }).ExecuteAsync(cancellationToken);

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
    }
}
