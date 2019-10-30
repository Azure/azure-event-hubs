// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Producer
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;
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
            var settings = new JsonSerializerSettings();

            settings.Converters.Add(new Newtonsoft.Json.Converters.StringEnumConverter());

            return settings;
        }
    }
}

