# Addition of Schema under Schema groups 

With release of [Schema registry](https://docs.microsoft.com/en-us/azure/event-hubs/schema-registry-overview#what-is-azure-schema-registry) feature in Event Hubs, it becomes easier to store different kinds of schemas under schema groups and have seamless interaction between producer and consumer application. Since addition of schema under schema groups is a data plane operation ( not intended to hit management endpoint) hence, there is no publicly available PS Cmdlet to abstract this functionality. 

This sample is meant to serve as reference code for adding schemas under schema groups using PowerShell. 

# Prerequisites

Please ensure that following steps are followed before following the sample code:

1. Updated PS module in Windows machine or you could use Azure CloudShell via https://shell.azure.com    

2. Create AD application for authentication,here are additional details: https://docs.microsoft.com/en-us/azure/active-directory/develop/howto-create-service-principal-portal 
3. Please keep a note of Application ID, secret that you had created in last step ( you would be using this in PS sample)
4. Assign the service principal Schema registry contributor role (To know more about how to assign a role: https://docs.microsoft.com/en-us/azure/role-based-access-control/role-assignments-portal?tabs=current ), so that it could make changes to schema groups. Here is quick read about Schema registry contributor role and permissions it posseses : https://docs.microsoft.com/en-us/azure/role-based-access-control/built-in-roles#schema-registry-contributor-preview 
5. Finally, update the following placeholders in sample code with information associated to your environment:
- Application_ID: Application ID of AD Application
- Application_Secret: Secret value associated to Application
- resource: needs to be set as "https://eventhubs.azure.net", since we are authenticating against data endpoint
- Tenant_Id: Tenant ID of Associated AD tenant
- Namespace_Name: Name of Event Hubs namespace
- SchemaGroup_Name: Name of schema groups under event hub namespace
- Schema_Name: Name of schema to be added under Schema group

# Script Overview

This PS sample can be spliced into two REST calls that are being made. 

1. Once we have the AD application details as talked above, we would make first API call to fetch the authorization token needed to successfully make schema addition call.
2. After fetching and formatting the bearer token, we trigger another API call to add schema under schema groups. Once this code runs successfully, you should be able to find the newly added schema under schema groups on Azure Portal. 

# Additional Reference:

Here are some additional reference links that could be helpful: 

- Schema Registry in Event Hubs - https://docs.microsoft.com/en-us/azure/event-hubs/schema-registry-overview 
- Event hubs Overview- https://docs.microsoft.com/en-us/azure/event-hubs/ 

# Conclusion

We would love to hear about your feedback about this sample. We would be happy to see any pull requests if in case you are interested to contribute to our community.

Stay tuned for future updates from Azure Messaging team. Happy scripting! 

