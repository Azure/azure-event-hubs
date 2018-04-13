# Step 1 – Login to your Azure subscription
Login-AzureRmAccount

# Optional – you need this if you do not have Event Hubs module already installed
Install-Module AzureRM.EventHub

# Optional - set your desired subscription
Set-AzureSubscription
   -SubscriptionName "your subscription name"

#Step 2 – following parameters are used while creating the resources
$location1 = “<provide your preferred primary location>”
$location2 = “<provide your preferred secondary location>”
$resourceGroup = “<provide your resource group name>”
$primarynamespace = “<provide your primary namespace name>”
$secondarynamespace = “<provide your secondary namespace name>”
$aliasname = “<provide your alias name>”

#Step 3 - Create Resource Group
New-AzureRmResourceGroup -Name $resourceGroup -Location $location1

#Step 4 - Create Event Hubs primary namespace
New-AzureRmEventHubNamespace – ResourceGroup $resourceGroup -NamespaceName $primarynamespace -Location $location1 -SkuName Standard

#Step 5 - Create Event Hubs secondary namespace
New-AzureRmEventHubNamespace – ResourceGroup $resourceGroup – NamespaceName $secondarynamespace -Location $location2 -SkuName Standard

#Step 6 – Creates an alias and pairs the namespaces
New-AzureRmEventHubGeoDRConfiguration -ResourceGroupName $resourcegroup -Name $aliasname -Namespace $primarynamespace -Namespace $secondarynamespace

#Optional – you can obtain the alias details using this cmdlet
Get-AzureRmEventHubGeoDRConfiguration -ResourceGroup $resourcegroup -Name $aliasname -Namespace $primarynamespace

#Optional – you can break the pairing between the two namespaces if you desire to associate a different pairing. Break pair is initiated over primary only
Set-AzureRmEventHubGeoDRConfigurationBreakPair -ResourceGroup $resourcegroup -Name $aliasname -Namespace $primarynamespace

#Optional – create entities to test the meta data sync between primary and secondary namespaces
#Here we are creating an event hub
New-AzureRmEventHub -ResourceGroup $resourcegroup -Namespace $primarynamespace -Name “testeventhub1” –location $location1

#Optional – check if the created event hub was replicated
Get-AzureRmEventHub  -ResourceGroup $resourcegroup -Namespace $secondarynamespace -Name “testeventhub1”

#Step 7 – Initiate a failover. This will break the pairing and alias will now point to the secondary namespace. Failover is initiated over secondary only
Set-AzureRmEventHubGeoDRConfigurationFailOver -ResourceGreoup $resourcegroup -Name $aliasname -Namespace $secondarynamespace

#Step 8 – Remove the Geo-pairing and its configuration. This cmdlet would delete the alias
Remove-AzureRmEventHubGeoDRConfiguration -ResourceGroup $resourcegroup -Name $aliasname

#Optional – clean up the created resoucegroup
Remove-AzureRmResourceGroup -Name $resourcegroup