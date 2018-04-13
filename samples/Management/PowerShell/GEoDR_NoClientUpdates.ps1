#Step1 - Login to you Azure subscription
Login-AzureRmAccount

#Option - set you desired subscription
Set-AzureRmContext -Subscription "your subscription name"

#Step2 - Following parameters are used while creating the resources
#geo location where the primary namespace is
$location1="SouthCentralUS"

#geo location where the secondary namespace should be created
$location2="NorthCentralUS"

#provide your existing resourcegroup which has the primary namespace
$resourceGroup = “<provide your resource group name>”

#provide your existing primary namespace
$primarynamespace = “<provide your primary namespace name>”

$secondarynamespace = “<provide your secondary namespace name>”

#your primary namespace is you alias here
$aliasname = “<provide your primary namespace name>”

$alternatename = "<your alternatename for you primary namespace>"

#Step 3 - create your secondary namespace. Copy the ARM ID from the output as you need it later for -PartnerPartnernamespace. 
# sample ARM ID looks like this - /subscriptions/your subscriptionid/resourceGroups/$resourcegroup/providers/Microsoft.EventHub/namespaces/secondarynamespace
New-AzureRmEventHubNamespace – ResourceGroup $resourceGroup – NamespaceName $secondarynamespace -Location $location2 -SkuName Standard

#Step 4 - Create a geo-dr configuration with primarynamespace name as the alias name and pair the namespaces. Note, you will also provide the alternatename here.
# It is using this alternatename that you will access your old primary once you have triggered the failover
New-AzureRmEventHubGeoDRConfiguration -Name $aliasname -Namespace $primarynamespace -PartnerNamespace "your ARM ID here" -ResourceGroupName $resourcegroup -AlternateName $alternatename 

#Optional - Once your geo-dr configurations are set, you can get the details using this'
Get-AzureRmEventHubGeoDRConfiguration -ResourceGroup $resourcegroup -Name $aliasname -Namespace $primarynamespace

#Step 5 - you can now do a failover on your secondary namespace with the following command
Set-AzureRmEventHubGeoDRConfigurationFailOver -ResourceGreoup $resourcegroup -Name $aliasname -Namespace $secondarynamespace

#Optional - check you geo-dr configuration details to reflect the fail-over
#Note - your secondarynamespace after the failover will be your new primary, but the connection string remains the same
Get-AzureRmEventHubGeoDRConfiguration -ResourceGroup $resourcegroup -Name $aliasname -Namespace $secondarynamespace 

