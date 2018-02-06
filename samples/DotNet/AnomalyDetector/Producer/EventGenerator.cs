// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Producer
{
    using System;
    using System.Collections.Generic;

    internal sealed class EventGenerator
    {
        // i.e. 1 in every 20 transactions is an anomaly
        private const int AnomalyFactor = 20;

        private const int MaxAmount = 1000;

        private const int MaxDays = 30;

        private readonly Random random = new Random((int)DateTimeOffset.UtcNow.Ticks);

        private readonly DateTimeOffset startTime = new DateTimeOffset(2017, 01, 01, 00, 00, 00, TimeSpan.Zero);

        private readonly List<string> knownCreditCards = new List<string>
        {
            "FC6018B3-B934-4FFF-B6A9-904C4B3082C7",
            "71B3B20F-0BD1-4C1D-BA33-3264D904581C",
            "B3B19755-C6E5-4E4A-B423-1B31DCCEB137",
            "1AF29CA0-FBF2-48B6-9D0B-88FF331FADC1",
            "039AA17C-2C5C-4A4F-B019-D416D0E4F5E6",
        };

        private readonly List<string> knownLocations = new List<string>
        {
            "Seattle",
            "San Francisco",
            "New York",
            "Kahului",
            "Miami",
        };

        public IEnumerable<Transaction> GenerateEvents(int count)
        {
            int counter = 0;

            var timestamp = startTime;

            while (counter < count)
            {                
                foreach (var t in NewMockPurchases(timestamp))
                {
                    yield return t;
                    counter++;
                }

                timestamp += TimeSpan.FromMinutes(10);
            }
        }

        /// <summary>
        /// Returns a new mock transaction. In some cases where there is an anomaly, then it returns
        /// 2 transactions. one regular and one anomaly. They differ in amounts, locations and timestamps, but have
        /// the same credit card Id.
        /// At the consumer side, since the timestamps are close together but the locations are different, these
        /// 2 transactions are flagged as anomalous.
        /// </summary>
        /// <returns></returns>
        private IEnumerable<Transaction> NewMockPurchases(DateTimeOffset timestamp)
        {
            var maxIndex = Math.Min(knownCreditCards.Count, knownLocations.Count);

            var index = random.Next(0, maxIndex);
            var cc = knownCreditCards[index];
            var location = knownLocations[index];

            bool isAnomaly = (random.Next(0, AnomalyFactor) % AnomalyFactor) == 0;

            var purchases = new List<Transaction>();
            
            var regularTransaction = new Transaction
            {
                Data = new TransactionData
                {
                    Amount = random.Next(1, MaxAmount),
                    Location = location,
                    CreditCardId = cc,
                    Timestamp = timestamp,
                },
                Type = TransactionType.Regular,
            };

            purchases.Add(regularTransaction);

            if (isAnomaly)
            {
                // change the location to something else
                // now the transaction on a credit card is happening from a different location which is an anomaly!

                string newLocation = null;

                do
                {
                    var newIndex = random.Next(0, knownLocations.Count);
                    newLocation = knownLocations[newIndex];

                    // while loop is - if by chance the "random" new location is the same as the original location

                } while (string.Equals(newLocation, location, StringComparison.OrdinalIgnoreCase));

                var suspectTransaction = new Transaction
                {
                    Data = new TransactionData
                    {
                        Amount = random.Next(1, MaxAmount),
                        Location = newLocation,
                        CreditCardId = cc,
                        Timestamp = timestamp + TimeSpan.FromSeconds(2), // suspect transaction time range is close to a regular transaction
                    },
                    Type = TransactionType.Suspect,
                };

                purchases.Add(suspectTransaction);
            }

            return purchases;
        }        
    }
}

