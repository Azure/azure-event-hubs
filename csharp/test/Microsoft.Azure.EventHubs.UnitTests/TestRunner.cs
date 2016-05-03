using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.Azure.EventHubs.UnitTests
{
    public static class TestRunner
    {
        static readonly object classLock = new object();
        static int failureCount;

        public static async Task RunAsync(Func<Task> testMethod)
        {
            try
            {
                await testMethod();
            }
            catch (Exception e)
            {
                lock(classLock)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine(e.ToString());
                    Console.ResetColor();
                    Interlocked.Increment(ref failureCount);
                }
            }
        }

        public static void Done()
        {
            if (failureCount > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"There were {failureCount} failures!");
                Console.ResetColor();
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"There were no failures.");
                Console.ResetColor();
            }
        }
    }
}
