namespace GeoDRClient
{
    internal class GeoDRConfig
    {
        public string SubscriptionId { get; set; }    

        public string PrimaryResourceGroupName { get; set; }

        public string SecondaryResourceGroupName { get; set; }

        public string SkuName { get; set; }

        public string ActiveDirectoryAuthority { get; set; }

        public string ResourceManagerUrl { get; set; }

        public string TenantId { get; set; }

        public string ClientId { get; set; }

        public string ClientSecrets { get; set; }

        public string PrimaryNamespace { get; set; }

        public string PrimaryNamespaceLocation { get; set; }

        public string SecondaryNamespace { get; set; }

        public string SecondaryNamespaceLocation { get; set; }

        public string Alias { get; set; }        
    }
}