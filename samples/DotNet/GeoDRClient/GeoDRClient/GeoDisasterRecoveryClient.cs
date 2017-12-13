using System;
using System.IO;
using System.Threading.Tasks;
using Microsoft.Azure.Management.EventHub;
using Microsoft.Azure.Management.EventHub.Models;
using Microsoft.IdentityModel.Clients.ActiveDirectory;
using Microsoft.Rest;

namespace GeoDRClient
{
    internal sealed class GeoDisasterRecoveryClient
    {
        public async Task<int> ExecuteAsync(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Invalid command");
                return -1;
            }
            
            string command = args[0];
            int status;

            try
            {                
                if (command.Equals("CreatePairing", StringComparison.OrdinalIgnoreCase))
                {
                    status = await CreatePairingAsync(args);
                }
                else if (command.Equals("FailOver", StringComparison.OrdinalIgnoreCase))
                {
                    status = await FailOverAsync(args);
                }
                else if (command.Equals("BreakPairing", StringComparison.OrdinalIgnoreCase))
                {
                    status = await BreakPairingAsync(args);
                }
                else if (command.Equals("DeleteAlias", StringComparison.OrdinalIgnoreCase))
                {
                    status = await DeleteAliasAsync(args);
                }
                else if (command.Equals("GetConnectionStrings", StringComparison.OrdinalIgnoreCase))
                {
                    status = await GetConnectionStringsAsync(args);
                }
                else
                {
                    Console.WriteLine("Invalid command");
                    status = -1;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error while executing '{command}'{Environment.NewLine}Exception: {ex}'");
                status = -2;
            }

            return status;
        }

        private async Task<EventHubManagementClient> GetClientAsync(GeoDRConfig config)
        {
            var token = await GetAuthorizationHeaderAsync(config);

            var creds = new TokenCredentials(token);
            var client = new EventHubManagementClient(creds) { SubscriptionId = config.SubscriptionId };
            return client;
        }
        
        private async Task<int> CreatePairingAsync(string[] args)
        {
            var config = LoadConfig(args[1]);
            var client = await GetClientAsync(config);

            // 1. Create Primary Namespace (optional)
            await CreateOrUpdateNamespaceAsync(
                client,
                config.PrimaryResourceGroupName,
                config.PrimaryNamespace,
                config.PrimaryNamespaceLocation,
                config.SkuName);

            //// 2. Create Secondary Namespace (optional if you already have an empty namespace available)
            var secondaryNamespaceInfo = await CreateOrUpdateNamespaceAsync(
                client,
                config.SecondaryResourceGroupName,
                config.SecondaryNamespace,
                config.SecondaryNamespaceLocation,
                config.SkuName);

            // 3. Pair the namespaces to enable DR.
            // If you re-run this program while namespaces are still paired this operation will fail with a bad request.
            // This is because we block all updates on secondary namespaces once it is paired
            ArmDisasterRecovery adr = await client.DisasterRecoveryConfigs.CreateOrUpdateAsync(
                config.PrimaryResourceGroupName,
                config.PrimaryNamespace,
                config.Alias,
                new ArmDisasterRecovery() { PartnerNamespace = secondaryNamespaceInfo.Id });

            while (adr.ProvisioningState != ProvisioningStateDR.Succeeded)
            {
                Console.WriteLine($"Waiting for DR to be setup. Current State: {adr.ProvisioningState}");

                adr = await client.DisasterRecoveryConfigs.GetAsync(
                    config.PrimaryResourceGroupName,
                    config.PrimaryNamespace,
                    config.Alias);

                await Task.Delay(TimeSpan.FromSeconds(30));
            }

            Console.WriteLine($"Pairing namespaces completed. Details: {Environment.NewLine}{adr.ToJson()}");

            // Perform verifications (optional)
            const string eventHubName = "myEH1";
            const string consumerGroupName = "myEH1-CG1";
            const int syncingDelay = 60;

            // Create an event hub and a consumer group within it. This is metadata and this metadata info replicates to the secondary namespace
            // since we have established the Geo DR pairing earlier.
            await client.EventHubs.CreateOrUpdateAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, eventHubName, new Eventhub());
            await client.ConsumerGroups.CreateOrUpdateAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, eventHubName, consumerGroupName, new ConsumerGroup());
            
            Console.WriteLine($"Waiting for {syncingDelay}s to allow metadata to sync across primary and secondary...");
            await Task.Delay(TimeSpan.FromSeconds(syncingDelay));

            // Verify that EventHubs and Consumer groups are present in secondary
            var accessingEHFromSecondary = await client.EventHubs.GetAsync(config.SecondaryResourceGroupName, config.SecondaryNamespace, eventHubName);
            Console.WriteLine($"{accessingEHFromSecondary.Name} successfully replicated");

            var accessingCGFromSecondary = await client.ConsumerGroups.GetAsync(config.SecondaryResourceGroupName, config.SecondaryNamespace, eventHubName, consumerGroupName);
            Console.WriteLine($"{accessingCGFromSecondary.Name} successfully replicated");

            return 0;
        }

        private async Task<int> FailOverAsync(string[] args)
        {
            var config = LoadConfig(args[1]);
            var client = await GetClientAsync(config);
            
            // We perform Failover here
            // Note that this Failover operations is ALWAYS run against the secondary (because primary might be down at time of failover)
            Console.WriteLine("Performing failover to secondary namespace...");
            await client.DisasterRecoveryConfigs.FailOverAsync(config.SecondaryResourceGroupName, config.SecondaryNamespace, config.Alias);
            Console.WriteLine("Failover successfully completed");

            // get alias connectionstrings
            var accessKeys = await client.Namespaces.ListKeysAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, "RootManageSharedAccessKey");
            var aliasPrimaryConnectionString = accessKeys.AliasPrimaryConnectionString;
            var aliasSecondaryConnectionString = accessKeys.AliasSecondaryConnectionString;

            return 0;
        }

        /// <summary>
        /// In a Geo DR enabled namespace, the Event Hubs can be accessed only via the alias.
        /// This is because, the alias can point to either the primary event hub or the failed over event hub
        /// This way, the user doesn't have to adjust the connection strings in his/her apps to point to a 
        /// different event hub in the case of a failover.
        /// </summary>
        private async Task<int> GetConnectionStringsAsync(string[] args)
        {
            var config = LoadConfig(args[1]);
            var client = await GetClientAsync(config);

            Console.WriteLine("Getting connection strings...");
            
            var accessKeys = await client.Namespaces.ListKeysAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, "RootManageSharedAccessKey");
            var aliasPrimaryConnectionString = accessKeys.AliasPrimaryConnectionString;
            var aliasSecondaryConnectionString = accessKeys.AliasSecondaryConnectionString;

            Console.WriteLine($"Alias primary connection string: {Environment.NewLine}{aliasPrimaryConnectionString}");
            Console.WriteLine($"Alias secondary connection string: {Environment.NewLine}{aliasSecondaryConnectionString}");

            return 0;
        }

        private async Task<string> GetAuthorizationHeaderAsync(GeoDRConfig config)
        {
            var context = new AuthenticationContext($"{config.ActiveDirectoryAuthority}/{config.TenantId}");

            AuthenticationResult result = await context.AcquireTokenAsync(
                config.ResourceManagerUrl,
                new ClientCredential(config.ClientId, config.ClientSecrets));

            if (result == null)
            {
                throw new InvalidOperationException("Failed to obtain token for authentication");
            }

            string token = result.AccessToken;
            return token;
        }

        private async Task<int> BreakPairingAsync(string[] args)
        {
            var config = LoadConfig(args[1]);
            var client = await GetClientAsync(config);

            await client.DisasterRecoveryConfigs.BreakPairingAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, config.Alias);

            return 0;
        }

        private async Task<int> DeleteAliasAsync(string[] args)
        {
            var config = LoadConfig(args[1]);
            var client = await GetClientAsync(config);

            // Before the alias is deleted, pairing needs to be broken
            await client.DisasterRecoveryConfigs.DeleteAsync(config.PrimaryResourceGroupName, config.PrimaryNamespace, config.Alias);

            return 0;
        }

        /// <summary>
        /// Loads a config file SPECIFIC to this sample program.
        /// </summary>
        private static GeoDRConfig LoadConfig(string jsonConfigFile)
        {
            if (jsonConfigFile == null)
            {
                throw new ArgumentNullException(nameof(jsonConfigFile));
            }

            if (string.IsNullOrWhiteSpace(jsonConfigFile))
            {
                throw new ArgumentException($"{nameof(jsonConfigFile)} cannot be null or whitespace");
            }

            var json = File.ReadAllText(jsonConfigFile);

            var o = json.FromJson<GeoDRConfig>();
            return o;
        }

        private static async Task<EHNamespace> CreateOrUpdateNamespaceAsync(
            EventHubManagementClient client,
            string rgName,
            string ns,
            string location,
            string skuName)
        {
            var namespaceParams = new EHNamespace
            {
                Location = location,
                Sku = new Sku
                {
                    Tier = skuName,
                    Name = skuName
                }
            };

            var namespace1 = await client.Namespaces.CreateOrUpdateAsync(rgName, ns, namespaceParams);
            Console.WriteLine($"{namespace1.Name} create/update completed");
            return namespace1;
        }
    }
}
