#Step 1- first step is to get the bearer token using client credential flow. We should set the resource to be URL encoded string for eventhubs.azure.net
response=$(curl -X POST -d  'grant_type=client_credentials&client_id=<Application_ID>&client_secret=<Application_Secret>&resource=https%3A%2F%2Feventhubs.azure.net' https://login.microsoftonline.com/<Tenant_Id>/oauth2/token)

#formatting token to drop "" quotes and suffixing bearer for final use
token="Bearer `echo $response | jq ."access_token" | tr -d '"'`"

#Step 2-Making a REST call to dataplane endpoint to add schema to schema groups
curl -X PUT -d '{"namespace": "com.azure.schemaregistry.samples","type": "record","name": "Order","fields": [{"name": "id","type": "string"},{"name": "amount","type": "double"}]}'  -H "Content-Type:application/json" -H "Authorization:$token" -H "Serialization-Type:Avro" \
'https://<Namespace_Name>.servicebus.windows.net/$schemagroups/<SchemaGroup_Name>/schemas/<Schema_Name>?api-version=2020-09-01-preview'