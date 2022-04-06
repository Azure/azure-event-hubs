
#Step 1 - Pass in AD Application details below to be able to fetch authorization token against legitimate Service Principal. 
$Fields = @{
    grant_type    = "client_credentials"
    client_id     = "<Application ID>"
    resource      = "https://eventhubs.azure.net"
    client_secret = "<Application Secret>"
  };

  
  $response = Invoke-RestMethod –Uri "https://login.microsoftonline.com/<Tenant_ID>/oauth2/token" –ContentType "application/x-www-form-urlencoded" –Method POST –Body $Fields

  #response would have bearer token in the properties that we would use to trigger next call.

  #Step 2- You can declare schema JSON associated to schema in $Body variable below
  $Body = @"
  {
    "namespace": "com.azure.schemaregistry.samples",
    "type": "record",
    "name": "Order",
    "fields": [
        {
            "name": "id",
            "type": "string"
        },
        {
            "name": "amount",
            "type": "double"
        }
    ]
}
"@ 

#Step 3- This Uri needs to have details added- namespace name, schema group name and schema name respectively.
$uri = 'https://<Namespace_Name>.servicebus.windows.net/$schemagroups/<SchemaGroup_Name>/schemas/<Schema_Name>?api-version=2020-09-01-preview'

#Step 4- These headers would be sent along with the API Call
$headers = @{
    "Content-Type" = "application/atom+xml;type=entry;charset=utf-8"
    "Serialization-Type" = "Avro"
    "Authorization" = "Bearer " + $response.access_token
}

#Step 5- This step shows the final call that we make to add schema under the schema groups, 
Invoke-RestMethod -Method "PUT" -Uri $uri -Headers $headers -Body $Body  -ContentType "application/json"
