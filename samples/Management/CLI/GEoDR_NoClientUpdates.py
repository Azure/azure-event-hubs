Following are the steps required to set up a geo-dr configuration with the existing connectiong strings

#Step1 - Login to your Azure subscription
az login

#Optional - set your desired subscription
az account set --subscription "your subscription name"

#Step 2 – following parameters are used while creating the resources. 

#geo location where the primarynamespace is
export location1=SouthCentralUS

#geo location where the secondarynamespace should be created
export location2=NorthCentralUS

#provide your exisitng resourcegroup which has the primarynamespace
export resourcegroup=myresourcegroup

#provide your existing primary namespace
export primarynamespace=myexistingprimarynamespace

export secondarynamespace=myexistingsecondarynamespace
# here your primarynamespace is your alias name
export aliasname=myexistingprimarynamespace
export alternatename=testehalternatename1

#Step 3 - create your secondary namespace. Copy the ARM ID from the output as you need it later for -PartnerPartnernamespace. 
# sample ARM ID looks like this - /subscriptions/your subscriptionid/resourceGroups/$resourcegroup/providers/Microsoft.EventHub/namespaces/secondarynamespace
az eventhubs namespace create --name $secondarynamespace --resource-group $resourcegroup --location $location2 --sku Standard

#Step 4 - Create a geo-dr configuration with primarynamespace name as the alias name and pair the namespaces. Note, you will also provide the alternatename here.
# It is using this alternatename that you will access your old primary once you have triggered the failover
az eventhubs georecovery-alias set --resource-group $resourcegroup --alias $primarynamespace --namespace-name $primarynamespace --partner-namespace "ARM ID of secondary namespace" --alternate-name $alternatename

#Optional - Once your geo-dr configurations are set, you can get the details using this
az eventhubs georecovery-alias show --resource-group $resourcegroup --alias $aliasname --namespace-name $primarynamespace

#Step 5 - you can now do a failover on your secondary namespace with the following command
az eventhubs georecovery-alias fail-over --resource-group $resourcegroup --alias $aliasname --namespace-name $secondarynamespace

#Optional - check you geo-dr configuration details to reflect the fail-over
#Note - your secondarynamespace after the failover will be your new primary, but the connection string remains the same
az eventhubs georecovery-alias show --resource-group $resourcegroup --alias $aliasname --namespace-name $secondarynamespace -- this is your current primary
