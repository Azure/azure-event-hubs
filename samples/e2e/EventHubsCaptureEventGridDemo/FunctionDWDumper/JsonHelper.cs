using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

namespace FunctionDWDumper
{
    public static class JsonHelper
    {
        private static readonly JsonSerializerSettings publicSerializationSettings = CreatePublicSerializationSettings();

        /// <summary>
        /// Converts an object to its Json representation.
        /// </summary>
        public static string ToJson<T>(this T value)
        {
            string json = JsonConvert.SerializeObject(value, typeof(T), publicSerializationSettings);
            return json;
        }

        /// <remarks>
        /// Converts a Json string to an object.
        /// </remarks>
        public static T FromJson<T>(this string value)
        {
            T @object = JsonConvert.DeserializeObject<T>(value, publicSerializationSettings);

            return @object;
        }

        private static JsonSerializerSettings CreatePublicSerializationSettings()
        {
            var settings = new JsonSerializerSettings
            {
                ContractResolver = new CamelCasePropertyNamesContractResolver()
            };

            return settings;
        }
    }
}