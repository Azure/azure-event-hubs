// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    // TODO: SIMPLIFY retryPolicy - ConcurrentHashMap is not needed
    public abstract class RetryPolicy
    {
        static readonly TimeSpan DEFAULT_RERTRY_MIN_BACKOFF = TimeSpan.Zero;
        static readonly TimeSpan DEFAULT_RERTRY_MAX_BACKOFF = TimeSpan.FromSeconds(30);

        const int DEFAULT_MAX_RETRY_COUNT = 10;

        static readonly RetryPolicy NO_RETRY = new RetryExponential(TimeSpan.Zero, TimeSpan.Zero, 0);

        protected RetryPolicy()
        {
            throw new NotImplementedException();
        }

        public void IncrementRetryCount(string clientId)
        {
            throw new NotImplementedException();
        }

        public void ResetRetryCount(string clientId)
        {
            throw new NotImplementedException();
        }

        public static bool IsRetryableException(Exception exception)
        {
            throw new NotImplementedException();
        }

        public static RetryPolicy Default
        {
            get
            {
                return new RetryExponential(
                    DEFAULT_RERTRY_MIN_BACKOFF,
                    DEFAULT_RERTRY_MAX_BACKOFF,
                    DEFAULT_MAX_RETRY_COUNT);
            }
        }

        public static RetryPolicy NoRetry
        {
            get
            {
                return RetryPolicy.NO_RETRY;
            }
        }

        protected int RetryCount(string clientId)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the Interval after which nextRetry should be done.
        /// </summary>
        /// <param name="clientId">the client id</param>
        /// <param name="lastException">the last exception</param>
        /// <param name="remainingTime">remaining time to retry</param>
        /// <returns>return null Duration when not Allowed.</returns>
        public abstract TimeSpan? GetNextRetryInterval(String clientId, Exception lastException, TimeSpan remainingTime);
    }
}