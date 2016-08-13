# Breaking changes in Microsoft Azure Event Hubs Client for Java

## version 0.8.0

*MessageAnnotations* like *iothub-connection-device-id*, *iothub-connection-auth-method* are moved out of *```EventData.getProperties()```* to *```EventData.getSystemProperties()```*.

*```SystemProperties EventData.getSystemProperties()```* is replaced by *```Map<String, Object> EventData.getSystemProperties()```*

*```EventData.getBody()```* is deprecated. Instead use *```EventData.getPayloadArray()```*.