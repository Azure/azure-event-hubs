// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs.Processor.UnitTests
{
    using System;

    public class Program
    {
        static void Main(string[] args)
        {
            Console.Write("Please enter an event hub connection string which includes an event hub name:");
            string eventHubConnectionString = Console.ReadLine().Trim();

            Console.Write("Please enter a storage connection string:");
            string storageConnectionString = Console.ReadLine().Trim();

            TestRunner.RunAsync(() => EventProcessorHostTests.RunAsync(eventHubConnectionString, storageConnectionString)).Wait();

            TestRunner.Done();
            Console.ReadLine();
        }
    }
}
