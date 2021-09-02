# EventHub Appender Java Sample

This sample demonstrates the use of EventHubAppender for streaming/sending log4j2 logs to Azure Event Hub.
The EventHubAppender extension (Log4j plugin) code can be found at https://github.com/Azure/azure-sdk-for-java/tree/main/sdk/eventhubs/microsoft-azure-eventhubs-extensions

Note : The above extension depends on [this version of Azure Event Hubs library](https://github.com/Azure/azure-sdk-for-java/tree/main/sdk/eventhubs/microsoft-azure-eventhubs).

Steps to run the project :
  1. Clone the repository.
  2. Update the value of 'eventHubConnectionString' in src/main/resources/log4j2.xml file. (This is the config file)
  3. Update the value of 'classpathPrefix' element in the pom.xml file of the project
  4. Run 'mvn clean'
  5. Run 'mvn package'
  6. Run 'java -cp target/my-app-1.0-SNAPSHOT.jar:<classpath-to-jar-dependencies>/*:  com.eventhubappendersample.app.App'
