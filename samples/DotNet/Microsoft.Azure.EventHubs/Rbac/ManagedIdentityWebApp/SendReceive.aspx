<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="SendReceive.aspx.cs" Inherits="ManagedIdentityWebApp.SendReceive" %>

<!DOCTYPE html>

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title>EventHubs Managed Identity Demo</title>
    <link rel="stylesheet" href="StyleSheet.css" />
</head>
<body>
    <form id="form1" runat="server">
        <div style="white-space: pre">
            <div>
                <label>Event Hubs Namespace</label><asp:TextBox ID="txtNamespace" runat="server" Text="" />
            </div>
            <div>
                <label>Event Hub Name</label><asp:TextBox ID="txtEventHub" runat="server" Text=""/>
            </div>
            <div>
                <label>Partition count</label><asp:TextBox ID="txtPartitions" runat="server"  Text="2"/>
            </div>
            <div>
                <label>Data to Send</label> <asp:TextBox ID="txtData" runat="server" TextMode="MultiLine" Width="500px"/>
            </div>
            <div>
                <asp:Button ID="btnSend" runat="server" Text="Send" OnClick="btnSend_Click" /> <asp:Button ID="btnReceive" runat="server" Text="Receive" OnClick="btnReceive_Click" />
            </div>            
            <div>
                <asp:TextBox ID="txtOutput" runat="server" Enabled="false" TextMode="MultiLine" Width="800px" Height="200px" />
                <asp:HiddenField ID="hiddenStartingOffset" runat="server" />
            </div>
        </div>
    </form>
</body>
</html>
