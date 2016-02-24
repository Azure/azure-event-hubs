<properties
  pageTitle="Create an Event Hub"
  description="Create an Event Hub to use with the sample programs"
  services="azure-iot"
  documentationCenter=".net"
  authors="dominicbetts"
  manager="timlt"
  editor=""/>

<tags
  ms.service="azure-iot"
  ms.workload="tbd"
  ms.tgt_pltfrm="na"
  ms.devlang="na"
  ms.topic="article"
  ms.date="06/23/2015"
  ms.author="dobett"/>

> This document is known to be outdated

# Create an Event Hub

To create an Event Hub to use with the sample applications, you need an active Azure account. <br/>If you don't have an account, you can create a free trial account in just a couple of minutes. For details, see [Azure Free Trial](http://azure.microsoft.com/pricing/free-trial).

To create an Event Hub:

1. Log on to the [Azure management portal](https://manage.windowsazure.com), and click **+ NEW** at the bottom of the screen.

2. Click **App Services**, then **Service Bus**, then **Event Hub**, then **Quick Create**.

  ![][1]

3. Type a name for your Event Hub, select your desired region, and then click **Create a new Event Hub**.

  ![][2]

  _Make a note of the name of the Event Hub you create._

4. Click the namespace you just created (usually **_event hub name_-ns**).

  ![][3]

5. Click the **Event Hubs** tab at the top of the page, and then click the Event Hub you just created.

  ![][4]

6. Click the **Configure** tab at the top of the page, add a rule named **SendRule** with *Send* permissions, add another rule called **ReceiveRule** with *Manage, Send, Listen* permissions, and then click **Save**.

  ![][5]

7. Click **Dashboard** at the top of the page, and then click **View Connection String**.

  ![][6]

  _Make a note of the **SendRule** and **ReceiveRule** connection strings._

Your Event Hub is now created, and you have the connection strings you need to send and receive events.

<!-- Images. -->
[1]: ./media/create-event-hub/create-event-hub1.png
[2]: ./media/create-event-hub/create-event-hub2.png
[3]: ./media/create-event-hub/create-event-hub3.png
[4]: ./media/create-event-hub/create-event-hub4.png
[5]: ./media/create-event-hub/create-event-hub5.png
[6]: ./media/create-event-hub/create-event-hub6.png
