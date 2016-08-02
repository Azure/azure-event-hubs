#Microsoft Azure Event Hubs C# Client Libraries

This subfolder contains open-source, client-side libraries for interacting with Microsoft Azure Event Hubs from C#.

To setup .NET Core (Core CLR) on your machine:

1. Install Visual Studio 2015

2. Run the "Visual Studio official MSI Installer" to install .NET Core RC2 and VS 2015 Tooling Preview 1:
https://go.microsoft.com/fwlink/?LinkId=798481

After installing .NET Core RC2 ensure to restart any command line windows in use since it updates the PATH environment variable.

These C# Event Hub libraries, Microsoft.Azure.EventHubs.dll and Microsoft.Azure.EventHubsProcessor.dll, require the Microsoft.Azure.Amqp.dll package for .NET Core.  Currently there is no published nuget package which contains this support.  Therefore in order to obtain this dependency one must follow these steps:

1. Clone the Azure/azure-amqp "develop" branch.  It is found at https://github.com/Azure/azure-amqp/tree/develop
2. Open the "develop" branch's solution, Microsoft\_Azure\_Amqp.sln.
3. Change the configuration to "Release" and build the solution (or just build Microsoft.Azure.Amqp and Microsoft.Azure.Amqp.Uwp projects).
4. In a command prompt change directory to "azure-amqp\Microsoft.Azure.Amqp\Nuget\" folder.
5. Run "powershell .\make_nuget_package.ps1"
6. Copy Microsoft.Azure.Amqp.2.0.0.nupkg to either "%PROGRAMFILES(X86)%\Microsoft SDKs\NuGetPackages\" or a custom folder on your machine which you have added to nuget's list of package sources (e.g. D:\Packages\ which is referenced by your nuget.config file).

NOTE: if you modify and rebuild Microsoft.Azure.Amqp.2.0.0.nupkg without changing the nuget package version (e.g. 2.0.0) then you must manually delete nuget's cached copy of the package at "%USERPROFILE%\.nuget\packages\Microsoft.Azure.Amqp\2.0.0".  Otherwise you will not get any new changes since the cached (previous) DLL will still be used.

Once Microsoft.Azure.Amqp.2.0.0.nupkg is available and can be located by nuget you will be able to build the .NET Core EventHub Client assemblies.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.