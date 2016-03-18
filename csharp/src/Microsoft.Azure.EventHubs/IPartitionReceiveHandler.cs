// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.EventHubs
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A handler interface for the receive operation. Use any implementation of this interface to specify
    /// user action when using <see cref="PartitionReceiver.SetReceiveHandler(IPartitionReceiveHandler)"/>.
    /// </summary>
    public interface IPartitionReceiveHandler
    {
        /// <summary>
        /// Users should implement this method to specify the action to be performed on the received events.
        /// </summary>
        /// <seealso cref="PartitionReceiver.ReceiveAsync"/>
        /// <param name="events">The list of fetched events from the corresponding PartitionReceiver.</param>
        void OnReceive(IEnumerable<EventData> events);

        void OnError(Exception error);

        void OnClose(Exception error);
    }
}
