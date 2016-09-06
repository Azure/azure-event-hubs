// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Concurrent;

    public abstract class RetryPolicy
    {
        const int DefaultRetryMaxCount = 10;

        static readonly TimeSpan DefaultRetryMinBackoff = TimeSpan.Zero;
        static readonly TimeSpan DefaultRetryMaxBackoff = TimeSpan.FromSeconds(30);

        ConcurrentDictionary<String, int> retryCounts;
        object serverBusySync;

        protected RetryPolicy()
        {
            this.retryCounts = new ConcurrentDictionary<string, int>();
            this.serverBusySync = new Object();
        }

        public void IncrementRetryCount(string clientId)
        {
            int retryCount;
            this.retryCounts.TryGetValue(clientId, out retryCount);
            this.retryCounts[clientId] = retryCount + 1;
        }

        public void ResetRetryCount(string clientId)
        {
            int currentRetryCount;
            this.retryCounts.TryRemove(clientId, out currentRetryCount);
        }

        public static bool IsRetryableException(Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            if (exception is ServiceBusException)
            {
                return ((ServiceBusException)exception).IsTransient;
            }

            // Take TimeoutException as transient. 
            // If the remaining time for the operation is too small then the client won't retry anyways.
            if (exception is TimeoutException)
            {
                return true;
            }

            return false;
        }

        public static RetryPolicy Default
        {
            get
            {
                return new RetryExponential(DefaultRetryMinBackoff, DefaultRetryMaxBackoff, DefaultRetryMaxCount);
            }
        }

        public static RetryPolicy NoRetry
        {
            get
            {
                return new RetryExponential(TimeSpan.Zero, TimeSpan.Zero, 0);
            }
        }

        protected int GetRetryCount(string clientId)
        {
            int retryCount;

            this.retryCounts.TryGetValue(clientId, out retryCount);

            return retryCount;
        }

        protected abstract TimeSpan? OnGetNextRetryInterval(String clientId, Exception lastException, TimeSpan remainingTime, int baseWaitTime);

        public TimeSpan? GetNextRetryInterval(string clientId, Exception lastException, TimeSpan remainingTime)
        {
            int baseWaitTime = 0;
            lock(this.serverBusySync)
            {
                if (lastException != null &&
                        (lastException is ServerBusyException || (lastException.InnerException != null && lastException.InnerException is ServerBusyException)))
			    {
                    baseWaitTime += ClientConstants.ServerBusyBaseSleepTimeInSecs;
                }
            }

            return this.OnGetNextRetryInterval(clientId, lastException, remainingTime, baseWaitTime);
        }
    }
}