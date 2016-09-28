// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Concurrent;
    using System.Threading;

    public enum RetryPolicyType
    {
        Default,
        NoRetry
    }

    public abstract class RetryPolicy
    {
        const int DefaultRetryMaxCount = 10;

        static readonly TimeSpan DefaultRetryMinBackoff = TimeSpan.Zero;
        static readonly TimeSpan DefaultRetryMaxBackoff = TimeSpan.FromSeconds(30);

        int retryCount;
        object serverBusySync;

        protected RetryPolicy()
        {
            this.retryCount = 0;
            this.serverBusySync = new Object();
        }

        public void IncrementRetryCount(string clientId)
        {
            Interlocked.Increment(ref this.retryCount);
        }

        public void ResetRetryCount()
        {
            this.retryCount = 0;
        }

        public int GetRetryCount()
        {
            return this.retryCount;
        } 

        public static bool IsRetryableException(Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            if (exception is EventHubsException)
            {
                return ((EventHubsException)exception).IsTransient;
            }

            return false;
        }

        public static RetryPolicy GetRetryPolicy(RetryPolicyType retryPolicyType)
        {
            switch (retryPolicyType)
            {
                case RetryPolicyType.Default:
                    return new RetryExponential(DefaultRetryMinBackoff, DefaultRetryMaxBackoff, DefaultRetryMaxCount);

                case RetryPolicyType.NoRetry:
                    return new RetryExponential(TimeSpan.Zero, TimeSpan.Zero, 0);
            }

            throw new NotImplementedException(string.Format("Retry policy implementation for {0}", retryPolicyType));
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