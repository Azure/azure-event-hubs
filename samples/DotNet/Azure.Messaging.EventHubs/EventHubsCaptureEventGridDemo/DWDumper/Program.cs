// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace DWDumper
{
    using System;
    using System.Data;
    using System.Data.SqlClient;
    using System.Text;
    using Avro.File;
    using Avro.Generic;
    using Newtonsoft.Json;
    using Azure.Storage.Blobs;
    using System.IO;

    /// <summary>
    /// A simple test program to dump a single Avro blob file created by EventHubs Capture into a SQL data warehouse (DW).
    /// This is useful for testing connections with your SQL DW before integrating this DW dumping code with Azure Functions
    /// </summary>
    class Program
    {
        private const string StorageConnectionString = "[provide your storage connection string]";
        private const string EventHubsCaptureAvroBlobContainer = "<< Blobs container>>";
        private const string EventHubsCaptureAvroBlobName = "<<Blob name>>";
        private const string SqlDwConnection = "[provide the SQL DW connection string]";


        private static int Main(string[] args)
        {
            var p = new Program();
            p.Dump();
            
            return 0;
        }

        public void Dump()
        {
            // Get the blob reference
            BlobContainerClient blobContainer = new BlobContainerClient(StorageConnectionString, EventHubsCaptureAvroBlobContainer);
            BlobClient blob = blobContainer.GetBlobClient(EventHubsCaptureAvroBlobName);

            // Download the content to a memory stream
            using (Stream blobStream = new MemoryStream())
            {
                blob.DownloadToAsync(blobStream);

                using (var dataTable = GetWindTurbineMetricsTable())
                {
                    // Parse the Avro File
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
                        BatchInsert(dataTable);
                    }
                }
            }
        }

        private void BatchInsert(DataTable table)
        {
            // Write the data to SQL DW using SqlBulkCopy
            using (var sqlDwConnection = new SqlConnection(SqlDwConnection))
            {
                sqlDwConnection.Open();

                using (var bulkCopy = new SqlBulkCopy(sqlDwConnection))
                {
                    bulkCopy.BulkCopyTimeout = 30;
                    bulkCopy.DestinationTableName = "dbo.Fact_WindTurbineMetrics";
                    bulkCopy.WriteToServer(table);
                }
            }
        }

        private WindTurbineMeasure DeserializeToWindTurbineMeasure(byte[] body)
        {
            string payload = Encoding.ASCII.GetString(body);
            return JsonConvert.DeserializeObject<WindTurbineMeasure>(payload);
        }

        private DataTable GetWindTurbineMetricsTable()
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

        private void AddWindTurbineMetricToTable(DataTable table, WindTurbineMeasure wtm)
        {
            table.Rows.Add(wtm.DeviceId, wtm.MeasureTime, wtm.GeneratedPower, wtm.WindSpeed, wtm.TurbineSpeed);
            Console.WriteLine(
                "DeviceId: {0}, MeasureTime: {1}, GeneratedPower: {2}, WindSpeed: {3}, TurbineSpeed: {4}",
                wtm.DeviceId, wtm.MeasureTime, wtm.GeneratedPower, wtm.WindSpeed, wtm.TurbineSpeed);
        }
    }

}
