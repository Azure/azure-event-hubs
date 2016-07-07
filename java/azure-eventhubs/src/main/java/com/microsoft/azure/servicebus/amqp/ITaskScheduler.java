/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.servicebus.amqp;

import org.apache.qpid.proton.engine.Handler;
import org.apache.qpid.proton.reactor.Task;

public interface ITaskScheduler
{
	Task schedule(int delay, Handler handler);
}
