#Microsoft Azure Event Hubs C# Client Libraries

This subfolder contains open-source, client-side libraries for interacting with Microsoft Azure Event Hubs from C#.

To setup .NET Core (Core CLR) on your machine:

1. Install Visual Studio 2015

2. Run the "Visual Studio official MSI Installer" to install .NET Core RC2 and VS 2015 Tooling Preview 1:
https://go.microsoft.com/fwlink/?LinkId=798481

After installing .NET Core RC2 ensure to restart any enlistment windows since it updates the PATH environment variable.

These C# Event Hub libraries, Microsoft.Azure.EventHubs.dll and Microsoft.Azure.EventHubsProcessor.dll, require the Microsoft.Azure.Amqp[.Core].dll package for .NET Core.  Currently there is no published nuget package which contains this support.  Therefore in order to obtain this dependency one must follow these steps:

1. Clone the Azure/azure-amqp "develop" branch.  It is found at https://github.com/Azure/azure-amqp/tree/develop
2. Open the "develop" branch's solution, Microsoft_Azure_Amqp.sln, and build the Microsoft.Azure.Amqp.Core project.  This will produce a package file at %REPOROOT%\bin\packages\Microsoft.Azure.Amqp.Core.1.1.0.nupkg.
3. Copy Microsoft.Azure.Amqp.Core.1.1.0.nupkg to either "%PROGRAMFILES(X86)%\Microsoft SDKs\NuGetPackages\" or a custom folder on your machine which you have added to nuget's list of package sources (e.g. D:\Packages\). 

NOTE: if you modify and rebuild Microsoft.Azure.Amqp.Core.dll without changing the nuget package version (e.g. 1.1.0) then you must manually delete nuget's cached copy of the package at "%USERPROFILE%\.nuget\packages\Microsoft.Azure.Amqp.Core\".  Otherwise you will not get any new changes since the cached(earlier) DLL will still be used.

Once Microsoft.Azure.Amqp.Core.1.1.0.nupkg is available and can be located by nuget one will be able to build the .NET Core EventHub Client assemblies.
