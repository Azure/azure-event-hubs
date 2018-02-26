# Step 1 – Login to your Azure subscription
Login-AzureRmAccount

# Optional – you need this if you do not have Event Hubs module already installed
Install-Module AzureRM.EventHub -Force

# Step 2 – Create the following resources
# Create a resource group, the following example uses "eventhubsResourceGroup" in East US region
New-AzureRmResourceGroup -Name eventhubsResourceGroup -Location eastus

# Create an Event Hubs namespace
New-AzureRmEventHubNamespace -ResourceGroupName eventhubsResourceGroup -NamespaceName <namespace_name> -Location eastus

# Get the connection string required to connect the clients to your event hub
$ehconnectionstring = Get-AzureRmEventHubKey -ResourceGroupName eventhubsResourceGroup -NamespaceName <namespace_name> -EventHubName <eventhub_name> -Name RootManageSharedAccessKey

Write-Host "event hubs connection string = " $ehconnectionstring

# Create storage account for Event Processor Host
# create a standard general-purpose storage account
New-AzureRmStorageAccount -ResourceGroupName eventhubsResourceGroup -Name <storage_account_name> -Location eastus -SkuName Standard_LRS

# retrieve the first storage account key for EPH client to connect
$storageAccountKey = (Get-AzureRmStorageAccountKey -ResourceGroupName $resourceGroup -Name $storageAccountName).Value[0]

Write-Host "storage account key 1 = " $storageAccountKey
