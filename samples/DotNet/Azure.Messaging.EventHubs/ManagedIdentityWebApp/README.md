# Managed Service Identity sample #
To use this sample, do the following steps: 

1. Create an Event Hubs namespace and an event hub.
2. Deploy the web app to Azure. 
3. Ensure that the SendReceive.aspx is set as the default document for the web app.
4. Enable identity for the web app.
5. Assign this identity to the Event Hubs Data Owner role at the namespace level or event hub level.
6. Run the web application, enter the namespace name and event hub name, a message, and select Send. To receive the event, select Receive.

    For more information on Managed Service Identity (MSI) and how to run this sample follow [this article](https://docs.microsoft.com/azure/event-hubs/authenticate-managed-identity#test-the-web-application).

