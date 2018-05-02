namespace FunctionEGDWDumper
{
    /// <summary>
    /// These classes were generated from the EventGrid event schema.
    /// <see cref="DWDumperFunction1"/> comments for understanding how the EventGrid schema was obtained.
    /// </summary>
    public class EventGridEHEvent
    {
        public string topic { get; set; }
        public string subject { get; set; }
        public string eventType { get; set; }
        public string eventTime { get; set; }
        public string id { get; set; }
        public Data data { get; set; }
    }

    public class Data
    {
        public string fileUrl { get; set; }
        public string fileType { get; set; }
        public string partitionId { get; set; }
        public int sizeInBytes { get; set; }
        public int eventCount { get; set; }
        public int firstSequenceNumber { get; set; }
        public int lastSequenceNumber { get; set; }
        public string firstEnqueueTime { get; set; }
        public string lastEnqueueTime { get; set; }
    }
}