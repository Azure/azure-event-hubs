#Step 1 – Login to your Azure subscription
az login
#Optional - set your desired subscription
az account set --subscription "your subscription name"

#Step 2 – following parameters are used while creating the resources. The variable declaration assumes you run the commands from a Powershell Window. You may need to change in case you ran e.g. from the portal.

export location1=NorthCentralUS
export location2=SouthCentralUS
export resourcegroup=geocliehtestrg
export primarynamespace=testgeocliprimaryns3
export secondarynamespace=testgeoclisecondaryns3
export aliasname=testgeoclialiasname3

#Step 3 - Create Resource Group
az group create --name $resourcegroup --location $location1

# Verfy the Primary namespae name is available
az eventhubs namespace exists --name $primarynamespace

# Verfy the Secondary namespae name is available
az eventhubs namespace exists --name $secondarynamespace

#Step 4 - Create the Event Hubs primary namespace
az eventhubs namespace create --name $primarynamespace --resource-group $resourcegroup --location $location1 --sku Standard

#Step 5 - Create the Event Hubs secondary namespace - Copy the ARM ID from the output as you need it later for -PartnerPartnernamespace. 
# sample ARM ID looks like this - /subscriptions/your subscriptionid/resourceGroups/$resourcegroup/providers/Microsoft.EventHub/namespaces/namespacenamesecondarycli
az eventhubs namespace create --name $secondarynamespace --resource-group $resourcegroup --location $location2 --sku Standard

#Step 6 – Creates an alias and pairs the namespaces
az eventhubs georecovery-alias set --resource-group $resourcegroup --alias $aliasname --namespace-name $primarynamespace --partner-namespace "---ARM ID from step before---"

#Optional – you can obtain the alias details using this cmdlet, here you can also check if the provisioning is done. Provisioning state should change from Accepted to Succeeded when any of the below or above operations are completed. Note that after failover you want to add the secondary namespace instead of the primary namespace.
az eventhubs georecovery-alias show --resource-group $resourcegroup --alias $aliasname --namespace-name $primarynamespace

#Optional – you can break the pairing between the two namespaces if you desire to associate a different pairing. Break pair is initiated over primary only
az eventhubs georecovery-alias break-pair --resource-group $resourcegroup --alias $aliasname --namespace-name $primarynamespace

#Optional – create entities to test the meta data sync between primary and secondary namespaces
az eventhubs eventhub create --resource-group $resourcegroup --namespace-name $primarynamespace --name testeventhub1

#Optional – check if the created queue was replicated
az eventhubs eventhub show --resource-group $resourcegroup --namespace-name $secondarynamespace --name testeventhub1

#Step 7 – Initiate a failover. This will break the pairing and alias will now point to the secondary namespace. Failover is initiated over secondary only
az eventhubs georecovery-alias fail-over --resource-group $resourcegroup --alias $aliasname --namespace-name $secondarynamespace

#Step 8 – Remove the Geo-pairing and its configuration. This cmdlet would delete the alias that is associated with the Geo-paired namespaces
# Deleting the alias would then break the pairing and at this point primary and secondary namespaces are no more in sync. you need to add the namespacename of either primary or secondary depending on if you failed over or not. Below shows the state after failover.
az eventhubs georecovery-alias delete --resource-group $resourcegroup --alias $aliasname --namespace-name $secondarynamespace

#Optional – clean up the created resoucegroup
az group delete --name $resourcegroup
