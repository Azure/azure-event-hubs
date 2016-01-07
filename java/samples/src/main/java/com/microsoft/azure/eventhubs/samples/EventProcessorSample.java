package com.microsoft.azure.eventhubs.samples;


import com.microsoft.azure.eventprocessorhost.*;

public class EventProcessorSample {
    public static void main(String args[])
    {
        EventProcessorHost host = new EventProcessorHost("namespace", "eventhub", "keyname",
                "key", "$Default", "storage connection string");
    }
}
