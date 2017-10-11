using System.Threading;
using Microsoft.Rest.Azure;

namespace GeoDRClient
{
    class Program
    {
        /// <summary>
        /// EventHubManagementLibrary.exe CreatePairing sampleconfig.json 
        /// </summary>
        static int Main(string[] args)
        {
            var client = new GeoDisasterRecoveryClient();
            int status = client.ExecuteAsync(args).GetAwaiter().GetResult();

            return status;
        }
    }
}