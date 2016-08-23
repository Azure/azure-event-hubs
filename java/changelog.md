# Release log of Microsoft Azure Event Hubs Client for Java

### 0.8.0
https://github.com/Azure/azure-event-hubs/milestone/5?closed=1

#### API Changes
##### New API
1. EventData.getBodyOffset() & EventData.getBodyLength()
2. EventData.getSystemProperties().getPublisher()
##### Deprecated API
1. EventData.setProperties()

#### Breaking Changes
1. MessageAnnotations on a received AMQPMessage are moved to EventData.getSystemProperties() as opposed to EventData.getProperties()
2. EventData.SystemProperties class now derives from HashSet<String, Object>. This can break serialized EventData.