# Azure.Messaging.EventHubs samples

## Anomaly Detector
In this sample, you run an application that creates and sends credit card transactions to an event hub. Then you read the stream of data in real time with Azure Stream Analytics, which separates the valid transactions from the invalid transactions, and then use Power BI to visually identify the transactions that are tagged as invalid.

For detailed information and steps for using this sample, see [this article](https://docs.microsoft.com/azure/event-hubs/event-hubs-tutorial-visualize-anomalies). 

## EventHubsCaptureEventGridDemo
This sample shows you how to capture data from your event hub into a SQL data warehouse by using an Azure function triggered by an event grid.

The WindTurbineGenerator app uses the new Azure.Messaging.EventHubs library to send data to an event hub.

For detailed information and steps for using this sample, see this [article](https://docs.microsoft.com/azure/event-hubs/store-captured-data-data-warehouse).

### ManagedIdentityWebApp
This sample provides a web application that you can use to send/receive events to/from an event hub using a managed identity. 

For more information on Managed Service Identity (MSI) and how to run this sample, follow [this article](https://docs.microsoft.com/azure/event-hubs/authenticate-managed-identity#test-the-web-application).
