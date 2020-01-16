
# Visualize data anomalies in real-time events sent to Azure Event Hubs
In this sample, you run an application that creates and sends credit card transactions to an event hub. Then you read the stream of data in real time with Azure Stream Analytics, which separates the valid transactions from the invalid transactions, and then use Power BI to visually identify the transactions that are tagged as invalid.

For detailed information and steps for using this sample, see [this article](https://docs.microsoft.com/azure/event-hubs/event-hubs-tutorial-visualize-anomalies). 

This sample uses the new Azure.Messaging.EventHubs library. If you're using the old Microsoft.Azure.EventHubs library, see the [old version of this sample](../../Microsoft.Azure.EventHubs/AnomalyDetector). 

