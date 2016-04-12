﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;

    class ExceptionUtility
    {
        internal ExceptionUtility()
        {
        }

        public ArgumentException Argument(string paramName, string message)
        {
            return new ArgumentException(message, paramName);
        }

        public Exception ArgumentNull(string paramName)
        {
            return new ArgumentNullException(paramName);
        }

        public ArgumentException ArgumentNullOrWhiteSpace(string paramName)
        {
            return Argument(paramName, Resources.ArgumentNullOrWhiteSpace.FormatForUser(paramName));
        }

        public ArgumentOutOfRangeException ArgumentOutOfRange(string paramName, object actualValue, string message)
        {
            return new ArgumentOutOfRangeException(paramName, actualValue, message);
        }

        public Exception AsError(Exception exception)
        {
            EventHubsEventSource.Log.ThrowingExceptionError(exception.ToString());
            return exception;
        }
    }
}
