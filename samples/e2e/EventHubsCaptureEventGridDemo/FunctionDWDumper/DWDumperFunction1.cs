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
using Microsoft.Azure.WebJobs.Extensions.Http;
using Microsoft.Azure.WebJobs.Host;
using Microsoft.WindowsAzure.Storage;
using Newtonsoft.Json;

namespace FunctionDWDumper
{    
    public static class DWDumperFunction1
    {
        private static readonly string StorageConnectionString = Environment.GetEnvironmentVariable("StorageConnectionString");
        private static readonly string SqlDwConnection = Environment.GetEnvironmentVariable("SqlDwConnection");

        /// <summary>
        /// Use the accompanying .sql script to create this table in the data warehouse
        /// </summary>
        private const string TableName = "dbo.Fact_WindTurbineMetrics";

        /// <summary>
        /// Before wiring this up with EventGrid, you can test this function by
        /// a. Create a new EventGrid subscription name to https://requestb.in
        ///    This keeps sending EventGrid json data to https://requestb.in
        /// b. From there, copy the json request body
        /// c. Execute the function in Azure portal as a one-off by selecting POST and providing the json body you copied
        /// d. This should create some rows in the data warehouse.
        ///    This ensures that the Azure Function is dumping data correctly to the data warehouse
        /// e. Next, create a new EventGrid subscription name to Azure Functions
        ///    This should automatically populate data to the data warehouse every time the Avro blob is created by Event Hubs Capture.
        /// </summary>
        /// <returns></returns>
        [FunctionName("HttpTriggerCSharp")]
        public static async Task<HttpResponseMessage> Run([HttpTrigger(AuthorizationLevel.Function, "get", "post", Route = null)]HttpRequestMessage req, TraceWriter log)
        {
            log.Info("C# HTTP trigger function processed a request.");

            try
            {
                string jsonContent = await req.Content.ReadAsStringAsync();

                EventGridEvent[] eventGridEvents = jsonContent.FromJson<EventGridEvent[]>();

                foreach (var ege in eventGridEvents)
                {
                    var uri = new Uri(ege.data.fileUrl);
                    Dump(uri);
                }

                return req.CreateResponse(HttpStatusCode.OK);
            }
            catch (Exception e)
            {
                // TODO, swallowing all exceptions for now!

                string s = string.Format(CultureInfo.InvariantCulture,
                    "Error processing request. Exception: {0}, Request: {1}", e, req.ToJson());
                log.Error(s);

                return req.CreateResponse(HttpStatusCode.InternalServerError);
            }
        }

        /// <summary>
        /// Dumps the data from the Avro blob to the data warehouse (DW). 
        /// Before running this, ensure that the DW has the required <see cref="TableName"/> table created.
        /// </summary>        
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