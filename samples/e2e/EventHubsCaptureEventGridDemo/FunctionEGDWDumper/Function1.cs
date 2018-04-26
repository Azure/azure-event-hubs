// This is the default URL for triggering event grid function in the local environment.
// http://localhost:7071/admin/extensions/EventGridExtensionConfig?functionName={functionname} 

using System;
using System.Data;
using System.Data.SqlClient;
using System.Globalization;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using Avro.File;
using Avro.Generic;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.EventGrid;
using Microsoft.Azure.WebJobs.Extensions.Http;
using Microsoft.Azure.WebJobs.Host;
using Microsoft.WindowsAzure.Storage;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace FunctionEGDWDumper
{

    public static class Function1
    {
        private static readonly string StorageConnectionString = Environment.GetEnvironmentVariable("StorageConnectionString");
        private static readonly string SqlDwConnection = Environment.GetEnvironmentVariable("SqlDwConnection");

        /// <summary>
        /// Use the accompanying .sql script to create this table in the data warehouse
        /// </summary>  
        private const string TableName = "dbo.Fact_WindTurbineMetrics";

        [FunctionName("Function1")]
        public static void Run([EventGridTrigger]JObject eventGridEvent, TraceWriter log)
        {
            log.Info("C# EventGrid trigger function processed a request.");
            log.Info(eventGridEvent.ToString(Formatting.Indented));
            // Copy to a static Album instance
            EventGridEHEvent ehEvent = eventGridEvent.ToObject<EventGridEHEvent>();

            var uri = new Uri(ehEvent.data.fileUrl);
            Dump(uri);
        }

        private static void Dump(Uri fileUri)
        {
            // Get the blob reference
            var storageAccount = CloudStorageAccount.Parse(StorageConnectionString);
            var blobClient = storageAccount.CreateCloudBlobClient();
            var blob = blobClient.GetBlobReferenceFromServer(fileUri);

            using (var dataTable = GetWindTurbineMetricsTable())
            {
                // Parse the Avro File
                using (var avroReader = DataFileReader<GenericRecord>.OpenReader(blob.OpenRead()))
                {
                    while (avroReader.HasNext())
                    {
                        GenericRecord r = avroReader.Next();

                        byte[] body = (byte[])r["Body"];
                        var windTurbineMeasure = DeserializeToWindTurbineMeasure(body);

                        // Add the row to in memory table
                        AddWindTurbineMetricToTable(dataTable, windTurbineMeasure);
                    }
                }

                if (dataTable.Rows.Count > 0)
                {
                    BatchInsert(dataTable);
                }
            }
        }
        private static void BatchInsert(DataTable table)
        {
            // Write the data to SQL DW using SqlBulkCopy
            using (var sqlDwConnection = new SqlConnection(SqlDwConnection))
            {
                sqlDwConnection.Open();

                using (var bulkCopy = new SqlBulkCopy(sqlDwConnection))
                {
                    bulkCopy.BulkCopyTimeout = 30;
                    bulkCopy.DestinationTableName = TableName;
                    bulkCopy.WriteToServer(table);
                }
            }
        }

        private static WindTurbineMeasure DeserializeToWindTurbineMeasure(byte[] body)
        {
            string payload = Encoding.ASCII.GetString(body);
            return JsonConvert.DeserializeObject<WindTurbineMeasure>(payload);
        }

        private static DataTable GetWindTurbineMetricsTable()
        {
            var dt = new DataTable();
            dt.Columns.AddRange
            (
                new DataColumn[5]
                {
                    new DataColumn("DeviceId", typeof(string)),
                    new DataColumn("MeasureTime", typeof(DateTime)),
                    new DataColumn("GeneratedPower", typeof(float)),
                    new DataColumn("WindSpeed", typeof(float)),
                    new DataColumn("TurbineSpeed", typeof(float))
                }
            );

            return dt;
        }

        private static void AddWindTurbineMetricToTable(DataTable table, WindTurbineMeasure wtm)
        {
            table.Rows.Add(wtm.DeviceId, wtm.MeasureTime, wtm.GeneratedPower, wtm.WindSpeed, wtm.TurbineSpeed);
            //string s = string.Format(
            //    CultureInfo.InvariantCulture,
            //    "DeviceId: {0}, MeasureTime: {1}, GeneratedPower: {2}, WindSpeed: {3}, TurbineSpeed: {4}",
            //    wtm.DeviceId, wtm.MeasureTime, wtm.GeneratedPower, wtm.WindSpeed, wtm.TurbineSpeed);            
        }
    }
}
