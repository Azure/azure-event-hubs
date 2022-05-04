// Default URL for triggering event grid function in the local environment.
// http://localhost:7071/runtime/webhooks/EventGrid?functionName={functionname}
using System;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.EventGrid;
using Microsoft.Extensions.Logging;

using System.Data;
using System.Data.SqlClient;
using System.Globalization;
using System.IO;
using System.Text;
using Avro.File;
using Avro.Generic;
using Azure.Storage.Blobs;
using Newtonsoft.Json;

using Azure.Messaging.EventGrid;
using Azure;
using Azure.Storage;

namespace FunctionEGDWDumper
{
    public static class Function1
    {
        private static readonly string StorageAccountName = Environment.GetEnvironmentVariable("StorageAccountName");
        private static readonly string StorageAccessKey = Environment.GetEnvironmentVariable("StorageAccessKey");
        private static readonly string SqlDwConnection = Environment.GetEnvironmentVariable("SqlDwConnection");

        /// <summary>
        /// Use the accompanying .sql script to create this table in the data warehouse
        /// </summary>  
        private const string TableName = "dbo.Fact_WindTurbineMetrics";

        [FunctionName("MyEventGridTriggerMigrateData")]
        public static void Run([EventGridTrigger]EventGridEvent eventGridEvent, ILogger log)
        {
            log.LogInformation("C# EventGrid trigger function processed a request.");
            log.LogInformation(eventGridEvent.Data.ToString());

            try
            {
                // Get the URL from the event that points to the Capture file
                Data data = eventGridEvent.Data.ToObjectFromJson<Data>();
                var uri = new Uri(data.fileUrl);
                log.LogInformation($"file URL: {data.fileUrl}");

                // Get data from the file and migrate to data warehouse
                Dump(uri, log);
            }
            catch (Exception e)
            {
                string s = string.Format(CultureInfo.InvariantCulture,
                    "Error processing request. Exception: {0}, Request: {1}", e, eventGridEvent.ToString());
                log.LogError(s);
            }
        }

        /// <summary>
        /// Dumps the data from the Avro blob to the data warehouse (DW). 
        /// Before running this, ensure that the DW has the required <see cref="TableName"/> table created.
        /// </summary>   
        private static async void Dump(Uri fileUri, ILogger log)
        {
            // Get the blob reference
            BlobClient blob = new BlobClient(fileUri, new StorageSharedKeyCredential(StorageAccountName, StorageAccessKey));

            using (var dataTable = GetWindTurbineMetricsTable())
            {
                // Parse the Avro File
                Stream blobStream = await blob.OpenReadAsync(null);
                using (var avroReader = DataFileReader<GenericRecord>.OpenReader(blobStream))
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
                    log.LogInformation("Batch insert into the dedicated SQL pool.");
                    BatchInsert(dataTable, log);
                }
            }
        }

        /// <summary>
        /// Open connection to data warehouse. Write the parsed data to the table. 
        /// </summary>   
        private static void BatchInsert(DataTable table, ILogger log)
        {
            // Write the data to SQL DW using SqlBulkCopy
            using (var sqlDwConnection = new SqlConnection(SqlDwConnection))
            {
                sqlDwConnection.Open();

                log.LogInformation("Bulk copying data.");
                using (var bulkCopy = new SqlBulkCopy(sqlDwConnection))
                {
                    bulkCopy.BulkCopyTimeout = 30;
                    bulkCopy.DestinationTableName = TableName;
                    bulkCopy.WriteToServer(table);
                }
            }
        }

        /// <summary>
        /// Deserialize data and return object with expected properties.
        /// </summary> 
        private static WindTurbineMeasure DeserializeToWindTurbineMeasure(byte[] body)
        {
            string payload = Encoding.ASCII.GetString(body);
            return JsonConvert.DeserializeObject<WindTurbineMeasure>(payload);
        }

        /// <summary>
        /// Define the in-memory table to store the data. The columns match the columns in the .sql script.
        /// </summary>   
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

        /// <summary>
        /// For each parsed record, add a row to the in-memory table.
        /// </summary>  
        private static void AddWindTurbineMetricToTable(DataTable table, WindTurbineMeasure wtm)
        {
            table.Rows.Add(wtm.DeviceId, wtm.MeasureTime, wtm.GeneratedPower, wtm.WindSpeed, wtm.TurbineSpeed);
        }
    }
}
