// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace EventHubsManagementSample
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Azure.Management.ResourceManager;
    using Microsoft.Azure.Management.ResourceManager.Models;
    using Microsoft.Azure.Management.EventHub;
    using Microsoft.Azure.Management.EventHub.Models;
    using Microsoft.Extensions.Configuration;
    using Microsoft.IdentityModel.Clients.ActiveDirectory;
    using Microsoft.Rest;

    public static class EventHubManagementSample
    {
        private const string EventHubName = "eventhub1";
        private const string ConsumerGroupName = "consumergroup1";

        private static readonly IConfigurationRoot SettingsCache;
        private static string tokenValue = string.Empty;
        private static DateTime tokenExpiresAtUtc = DateTime.MinValue;

        private static string resourceGroupName = string.Empty;
        private static string namespaceName = string.Empty;

        static EventHubManagementSample()
        {
            SettingsCache = new ConfigurationBuilder()
                             .AddJsonFile("appsettings.json", true, true)
                             .Build();
        }

        public static void Run()
        {
            CreateResourceGroup().ConfigureAwait(false).GetAwaiter().GetResult();
            CreateNamespace().ConfigureAwait(false).GetAwaiter().GetResult();
            CreateEventHub().ConfigureAwait(false).GetAwaiter().GetResult();
            CreateConsumerGroup().ConfigureAwait(false).GetAwaiter().GetResult();

            Console.WriteLine("Press a key to exit.");
            Console.ReadLine();
        }

        private static async Task CreateResourceGroup()
        {
            try
            {
                Console.WriteLine("What would you like to call your resource group?");
                resourceGroupName = Console.ReadLine();

                var token = await GetToken();

                var creds = new TokenCredentials(token);
                var rmClient = new ResourceManagementClient(creds)
                {
                    SubscriptionId = SettingsCache["SubscriptionId"]
                };

                var resourceGroupParams = new ResourceGroup()
                {
                    Location = SettingsCache["DataCenterLocation"],
                    Name = resourceGroupName,
                };

                Console.WriteLine("Creating resource group...");
                await rmClient.ResourceGroups.CreateOrUpdateAsync(resourceGroupName, resourceGroupParams);
                Console.WriteLine("Created resource group successfully.");
            }
            catch (Exception e)
            {
                Console.WriteLine("Could not create a resource group...");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        private static async Task CreateNamespace()
        {
            try
            {
                Console.WriteLine("What would you like to call your Event Hubs namespace?");
                namespaceName = Console.ReadLine();

                var token = await GetToken();

                var creds = new TokenCredentials(token);
                var ehClient = new EventHubManagementClient(creds)
                {
                    SubscriptionId = SettingsCache["SubscriptionId"]
                };

                var namespaceParams = new EHNamespace()
                {
                    Location = SettingsCache["DataCenterLocation"]
                };

                Console.WriteLine("Creating namespace...");
                await ehClient.Namespaces.CreateOrUpdateAsync(resourceGroupName, namespaceName, namespaceParams);
                Console.WriteLine("Created namespace successfully.");
            }
            catch (Exception e)
            {
                Console.WriteLine("Could not create a namespace...");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        private static async Task CreateEventHub()
        {
            try
            {
                if (string.IsNullOrEmpty(namespaceName))
                {
                    throw new Exception("Namespace name is empty!");
                }

                var token = await GetToken();

                var creds = new TokenCredentials(token);
                var ehClient = new EventHubManagementClient(creds)
                {
                    SubscriptionId = SettingsCache["SubscriptionId"]
                };

                var ehParams = new Eventhub() { };

                Console.WriteLine("Creating Event Hub...");
                await ehClient.EventHubs.CreateOrUpdateAsync(resourceGroupName, namespaceName, EventHubName, ehParams);
                Console.WriteLine("Created Event Hub successfully.");
            }
            catch (Exception e)
            {
                Console.WriteLine("Could not create an Event Hub...");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        private static async Task CreateConsumerGroup()
        {
            try
            {
                if (string.IsNullOrEmpty(namespaceName))
                {
                    throw new Exception("Namespace name is empty!");
                }

                var token = await GetToken();

                var creds = new TokenCredentials(token);
                var ehClient = new EventHubManagementClient(creds)
                {
                    SubscriptionId = SettingsCache["SubscriptionId"]
                };

                var consumerGroupParams = new ConsumerGroup() { };

                Console.WriteLine("Creating Consumer Group...");
                await ehClient.ConsumerGroups.CreateOrUpdateAsync(resourceGroupName, namespaceName, EventHubName, ConsumerGroupName, consumerGroupParams);
                Console.WriteLine("Created Consumer Group successfully.");
            }
            catch (Exception e)
            {
                Console.WriteLine("Could not create a Consumer Group...");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        private static async Task<string> GetToken()
        {
            try
            {
                // Check to see if the token has expired before requesting one.
                // We will go ahead and request a new one if we are within 2 minutes of the token expiring.
                if (tokenExpiresAtUtc < DateTime.UtcNow.AddMinutes(-2))
                {
                    Console.WriteLine("Renewing token...");

                    var tenantId = SettingsCache["TenantId"];
                    var clientId = SettingsCache["ClientId"];
                    var clientSecret = SettingsCache["ClientSecret"];

                    var context = new AuthenticationContext($"https://login.windows.net/{tenantId}");

                    var result = await context.AcquireTokenAsync(
                        "https://management.core.windows.net/",
                        new ClientCredential(clientId, clientSecret)
                    );

                    // If the token isn't a valid string, throw an error.
                    if (string.IsNullOrEmpty(result.AccessToken))
                    {
                        throw new Exception("Token result is empty!");
                    }

                    tokenExpiresAtUtc = result.ExpiresOn.UtcDateTime;
                    tokenValue = result.AccessToken;
                    Console.WriteLine("Token renewed successfully.");
                }

                return tokenValue;
            }
            catch (Exception e)
            {
                Console.WriteLine("Could not get a new token...");
                Console.WriteLine(e.Message);
                throw e;
            }
        }
    }
}