// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.UnitTests
{
    using System;

    public class Program
    {
        static void Main(string[] args)
        {
            string connectionString;
            if (args.Length == 0 || string.IsNullOrEmpty((connectionString = args[0])))
            {
                Console.Write("Please enter a connection string which includes an event hub name:");
                connectionString = Console.ReadLine().Trim();
            }

            TestRunner.RunAsync(() => EventHubClientTests.RunAsync(connectionString)).Wait();

            TestRunner.Done();
            Console.ReadLine();
        }
    }
}
