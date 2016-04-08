// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System.Diagnostics;

    static class Fx
    {
        static ExceptionUtility exceptionUtility;

        public static ExceptionUtility Exception
        {
            get
            {
                if (exceptionUtility == null)
                {
                    exceptionUtility = new ExceptionUtility();
                }

                return exceptionUtility;
            }
        }

        [Conditional("DEBUG")]
        public static void Assert(bool condition, string message)
        {
            Debug.Assert(condition, message);
        }
    }
}
