/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.servicebus;

import org.apache.qpid.proton.engine.Session;

interface ISessionProvider
{
	Session getSession(final String sessionId);
}
