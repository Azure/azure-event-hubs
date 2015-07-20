# Using the CSharp_ConsumeEventsFromEventHub tool

You can use this simple tool to receive events from an Event Hub. This is useful to verify the behavior of the **send** and **send_batch** samples in this repository.

To use this tool:

1. If you have not already done so, create an Event Hub to use with the sample applications by following the steps in [Create an Event Hub](../../docs/create_event_hub.md).

2. Open the **CSharp_ConsumeEventsFromEventHub** solution in Visual Studio 2013.

3. Replace the connection information placeholders in program.cs with your **ManageRule** connection string and the name of your Event Hub.

4. Build and run the tool. Building the tool will automatically download and install the prerequisite NuGet packages.
