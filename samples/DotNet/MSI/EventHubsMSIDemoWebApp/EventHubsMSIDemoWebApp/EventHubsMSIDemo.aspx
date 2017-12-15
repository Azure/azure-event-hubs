<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="EventHubsMSIDemo.aspx.cs" Inherits="EventHubsMSIDemoWebApp.EventHubsMSIDemo" %>

<!DOCTYPE html>

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title>EventHubs MSI Demo</title>
    <link rel="stylesheet" href="StyleSheet.css" />
</head>
<body>
    <form id="form1" runat="server">
        <div style="white-space: pre">
            <div>
                <label>Namespace </label><asp:TextBox ID="txtNamespace" runat="server" Text="skulkarnrbactest1" />
            </div>
            <div>
                <label>EventHub </label><asp:TextBox ID="txtEventHub" runat="server" Text="skulkar-rbac-eh"/>
            </div>
            <div>
                <label>Partition count </label><asp:TextBox ID="txtPartitions" runat="server"  Text="2"/>
            </div>
            <div>
                <label>Data to Send </label> <asp:TextBox ID="txtData" runat="server" TextMode="MultiLine" Width="500px"/>
            </div>
            <div>
                <asp:Button ID="btnSend" runat="server" Text="Send" OnClick="btnSend_Click" /> <asp:Button ID="btnReceive" runat="server" Text="Receive" OnClick="btnReceive_Click" />
            </div>            
            <div>
                <label>Received Data </label> <asp:TextBox ID="txtReceivedData" runat="server" Enabled="false" TextMode="MultiLine" Width="500px" Height="100px" />
                <asp:HiddenField ID="hiddenStartingOffset" runat="server" />
            </div>
        </div>
    </form>
</body>
</html>
