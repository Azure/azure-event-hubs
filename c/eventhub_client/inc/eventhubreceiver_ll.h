// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** 
*   @file eventhubreceiver_ll.h
*	@brief The EventHubReceiver_LL module implements receive operations 
*          related to a Event Hub consumer group partition.
*
*	@details EventHubReceiver_LL is a module that can be used for 
*            performing Event Hub receive IO operations in a non 
*            multi-threaded environment and used to schedule work
*            in a work loop type system.
*/

#ifndef EVENTHUBRECEIVER_LL_H
#define EVENTHUBRECEIVER_LL_H

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#include <stdint.h>
#endif

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "eventdata.h"

#define EVENTHUBRECEIVER_RESULT_VALUES                \
        EVENTHUBRECEIVER_OK,                          \
        EVENTHUBRECEIVER_INVALID_ARG,                 \
        EVENTHUBRECEIVER_ERROR,                       \
        EVENTHUBRECEIVER_TIMEOUT,                     \
        EVENTHUBRECEIVER_CONNECTION_RUNTIME_ERROR,    \
        EVENTHUBRECEIVER_NOT_ALLOWED

DEFINE_ENUM(EVENTHUBRECEIVER_RESULT, EVENTHUBRECEIVER_RESULT_VALUES);

/**
* @brief	An opaque handle used by clients of EventHubReceiver_LL for
*           purposes of communication with an existing Event Hub.
*/
typedef struct EVENTHUBRECEIVER_LL_TAG* EVENTHUBRECEIVER_LL_HANDLE;

/**
* @brief	A callback definition for asynchronous callback used by
*           clients of EventHubReceiver_LL for purposes of communication
*           with an existing Event Hub. This callback will be invoked
*           when an event is received (read from) an event hub partition.
*
* @param	result              Receive operation result. Expected return
*                               codes are EVENTHUBRECEIVER_OK on a successful
*                               event receive and EVENTHUBRECEIVER_TIMEOUT if
*                               a timeout has occurred and no event data has been
*                               received.
* @param	dataBuffer          Data buffer containing the payload from
*                               the event hub
* @param	dataSize            Data buffer size
* @param    userContext         A user supplied context
*
* @return   None
*/
typedef void(*EVENTHUBRECEIVER_ASYNC_CALLBACK)(EVENTHUBRECEIVER_RESULT result,
                                                EVENTDATA_HANDLE eventDataHandle,
                                                void* userContext);

/**
* @brief	A callback definition for asynchronous callback used by
*           clients of EventHubReceiver_LL for purposes of communication
*           with an existing Event Hub. This callback will be invoked
*           when an unexpected error has occurred during a receive 
*           operation from an event hub partition. This callback provides
*           the user the capability to gracefully handle an error.
*
* @param	errorCode           Unexpected error code
* @param    userContext         A user supplied context
*
* @return   None
*/
typedef void(*EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK)(EVENTHUBRECEIVER_RESULT errorCode,
                                                        void* userContextCallback);

/**
* @brief	A callback definition for asynchronous callback used by
*           clients of EventHubReceiver_LL for purposes of tearing down
*           communication with an existing Event Hub. This callback will
*           be invoked after the tear down has been completed.
*
* @param	errorCode           Unexpected error code
* @param    userContext         A user supplied context
*
* @return   None
*/
typedef void(*EVENTHUBRECEIVER_ASYNC_END_CALLBACK)(EVENTHUBRECEIVER_RESULT result,
                                                                void* userContextCallback);

/**
* @brief	Creates a EventHubReceiver_LL handle for communication with an
* 			existing Event Hub using the specified parameters for the purposes
*           of reading event data posted on a specific partition within the
*           Event Hub.
*
* @param	connectionString	Pointer to a character string containing the connection string (see below)
* @param	eventHubPath        Pointer to a character string identifying the Event Hub path
* @param	consumerGroup       Pointer to a character string identifying a specific consumer group within a Event hub
* @param	partitionId         Pointer to a character string containing the specific ID for a partition within the event hub
*
* Sample connection string:
*  <blockquote>
*    <pre>HostName=[Event Hub name goes here].[Event Hub suffix goes here, e.g., servicebus.windows.net];SharedAccessKeyName=[Policy key name here];SharedAccessKey=[Policy key];EntityPath=[Event Hub Path]</pre>
*  </blockquote>
*
* @return	A non-NULL @c EVENTHUBRECEIVER_LL_HANDLE value that is used 
*           when invoking other functions for EventHubReceiver
*           @c NULL on failure.
*/
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_LL_HANDLE, EventHubReceiver_LL_Create,
    const char*,  connectionString,
    const char*,  eventHubPath,
    const char*,  consumerGroup,
    const char*,  partitionId
);

/**
* @brief	Disposes of resources allocated by the EventHubReceiver_Create_LL.
*
* @note     This is a blocking call.
*
* @param	eventHubReceiverHandle	The handle created by a call to the create function.
*
* @return	None
*/
MOCKABLE_FUNCTION(, void, EventHubReceiver_LL_Destroy, EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverHandle);

/**
* @brief	Asynchronous call to receive events message specified by @p eventHubReceiverHandle.
*
* @param	eventHubReceiverLLHandle    The handle created by a call to the create function.
* @param	onEventReceiveCallback  	The callback specified by the user for receiving
* 										event payload from the Event Hub. If there is event
*                                       data to be read, the callback result code will be
*                                       EVENTHUBRECEIVER_OK.
* @param	onEventReceiveUserContext   User specified context that will be provided to the
* 										callback. This can be @c NULL.
* @param	onEventReceiveErrorCallback The callback specified by the user for receiving
* 										notifications of an unexpected error that might have
*                                       occurred during a receive operation from the Event Hub.
*                                       This will give the user an opportunity to manage this
*                                       situation such as cleaning up of resources etc.
* @param	onEventReceiveUserContext   User specified context that will be provided to the
* 										callback. This can be @c NULL.
* @param    startTimestampInSec         Timestamp from UTC epoch in units of seconds from which
*                                       to read data from the partition in the Event Hub.
*
*			@b NOTE: The application behavior is undefined if the user calls
*			the EventHubReceiver_Close_LL function from within any callback.
*
* @return	EVENTHUBRECEIVER_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveFromStartTimestampAsync,
	EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle, 
	EVENTHUBRECEIVER_ASYNC_CALLBACK, onEventReceiveCallback, 
	void*, onEventReceiveUserContext, 
	EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, onEventReceiveErrorCallback,
	void*, onEventReceiveErrorUserContext,
    uint64_t, startTimestampInSec
);

/**
* @brief	Asynchronous call to receive events message specified by @p eventHubReceiverHandle.
*           Additionally users can specify a timeout value in milliseconds to wait in case there
*           are no more events to read from an Event Hub partition.
*
* @param	eventHubReceiverLLHandle    The handle created by a call to the create function.
* @param	onEventReceiveCallback  	The callback specified by the user for receiving
* 										event payload from the Event Hub. If there is event
*                                       data to be read, the callback result code will be
*                                       EVENTHUBRECEIVER_OK. If a timeout has occurred,
*                                       the callback result code will be EVENTHUBRECEIVER_TIMEOUT.
* @param	onEventReceiveUserContext   User specified context that will be provided to the
* 										callback. This can be @c NULL.
* @param	onEventReceiveErrorCallback The callback specified by the user for receiving
* 										notifications of an unexpected error that might have
*                                       occurred during a receive operation from the Event Hub.
*                                       This will give the user an opportunity to manage this
*                                       situation such as cleaning up of resources etc.
* @param	onEventReceiveUserContext   User specified context that will be provided to the
* 										callback. This can be @c NULL.
* @param    startTimestampInSec         Timestamp from UTC epoch in units of seconds from which
*                                       to read data from the partition in the Event Hub.
* @param    waitTimeoutInMs             Timeout wait period specified in milliseconds.
*                                       This is useful in cases when the user would like to get
*                                       notified due to inactivity for the specified wait period.
*                                       On expiration of the timeout period, the registered
*                                       onEventReceiveCallback will be invoked with result
*                                       parameter as EVENTHUBRECEIVER_TIMEOUT. Passing in a
*                                       value of zero will result in no timeouts and the behavior
*                                       will be similar to
*                                       EventHubReceiver_ReceiveFromStartTimestampAsync_LL.
*
*			@b NOTE: The application behavior is undefined if the user calls
*			the EventHubReceiver_Close function from within any callback.
*
* @return	EVENTHUBRECEIVER_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync,
	EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle,
	EVENTHUBRECEIVER_ASYNC_CALLBACK, onEventReceiveCallback,
	void*, onEventReceiveUserContext,
	EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, onEventReceiveErrorCallback,
	void*, onEventReceiveErrorUserContext,
    uint64_t, startTimestampInSec,
    unsigned int, waitTimeoutInMs
);

/**
* @brief	API to asynchronously cleanup and terminate communication with the event hub
*
* @param	eventHubReceiverLLHandle        The handle created by a call to the create function.
* @param	onEventReceiveEndCallback  	    The callback specified by the user notifying the
*                                           user that the communication to the event hub
*                                           has terminated. On success, the callback result
*                                           code will be EVENTHUBRECEIVER_OK and an error
*                                           code otherwise. This is an optional parameter and
*                                           therefore can be @c NULL.
* @param	onEventReceiveEndUserContext    User specified context that will be provided to the
* 										    callback. This can be @c NULL.
*
* @return	EVENTHUBRECEIVER_OK upon success or an error code upon failure.
*
* @note     This API is safe to call within a EVENTHUBRECEIVER_ASYNC_CALLBACK or
*           EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK callback.
*
*           In the registered callback below, users may call EventHubReceiver_Destroy_LL
*/
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_ReceiveEndAsync,
    EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK, onEventReceiveEndCallback,
    void*, onEventReceiveEndUserContext
);

/**
* @brief	API to perform the actual work of reading event data from the event hub data from a
*           partition and issuing the relevant callbacks to clients waiting for event data and 
*           handling timeouts. This function is expected to be called by runtime environments
*           that control the schedule of work for a specific handle.
*
* @param	eventHubReceiverLLHandle	The handle created by a call to the create function.
*
* @return	EVENTHUBRECEIVER_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, void, EventHubReceiver_LL_DoWork, EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle);

/**
* @brief	API to enable diagnostic tracing related to connection establishing, data transfer
*           and tear down.
*
* @param	eventHubReceiverLLHandle	The handle created by a call to the create function.
* @param	traceEnabled              	True to enable tracing and false to disable tracing
*
* @return	EVENTHUBRECEIVER_OK upon success or an error code upon failure.
*
* @note By default tracing is disabled.
*/
MOCKABLE_FUNCTION(, EVENTHUBRECEIVER_RESULT, EventHubReceiver_LL_SetConnectionTracing,
                    EVENTHUBRECEIVER_LL_HANDLE, eventHubReceiverLLHandle, bool, traceEnabled);

#ifdef __cplusplus
}
#endif

#endif // EVENTHUBRECEIVER_LL_H
