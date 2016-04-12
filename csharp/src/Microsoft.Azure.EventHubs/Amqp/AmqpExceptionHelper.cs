using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.Azure.Amqp;
using Microsoft.Azure.Amqp.Framing;

namespace Microsoft.Azure.EventHubs.Amqp
{
    public class AmqpExceptionHelper
    {
        public static Exception ToMessagingContract(Error error, bool connectionError = false)
        {
            throw new NotImplementedException(error.Description + ": " + error.Condition.Value);
            //if (error == null)
            //{
            //    return new MessagingException("Unknown error.");
            //}

            //return ToMessagingContract(error.Condition.Value, error.Description, connectionError);
        }

        //public static Exception ToMessagingContract(string condition, string message, bool connectionError = false)
        //{
        //    if (string.Equals(condition, AmqpClientConstants.TimeoutError.Value))
        //    {
        //        return new TimeoutException(message);
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.NotFound.Value))
        //    {
        //        if (connectionError)
        //        {
        //            return new MessagingCommunicationException(message, null);
        //        }
        //        else
        //        {
        //            return new MessagingEntityNotFoundException(message, null);
        //        }
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.NotImplemented.Value))
        //    {
        //        return new NotSupportedException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.EntityAlreadyExistsError.Value))
        //    {
        //        return new MessagingEntityAlreadyExistsException(message, null, null);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.MessageLockLostError.Value))
        //    {
        //        return new MessageLockLostException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.SessionLockLostError.Value))
        //    {
        //        return new SessionLockLostException(message);
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.ResourceLimitExceeded.Value))
        //    {
        //        return new Microsoft.ServiceBus.Messaging.QuotaExceededException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.NoMatchingSubscriptionError.Value))
        //    {
        //        return new NoMatchingSubscriptionException(message);
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.NotAllowed.Value))
        //    {
        //        return new InvalidOperationException(message);
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.UnauthorizedAccess.Value))
        //    {
        //        return new UnauthorizedAccessException(message);
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.MessageSizeExceeded.Value))
        //    {
        //        return new MessageSizeExceededException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.ServerBusyError.Value))
        //    {
        //        return new ServerBusyException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.ArgumentError.Value))
        //    {
        //        return new ArgumentException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.ArgumentOutOfRangeError.Value))
        //    {
        //        return new ArgumentOutOfRangeException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.StoreLockLostError.Value))
        //    {
        //        return new MessageStoreLockLostException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.SessionCannotBeLockedError.Value))
        //    {
        //        return new SessionCannotBeLockedException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.PartitionNotOwnedError.Value))
        //    {
        //        return new PartitionNotOwnedException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.EntityDisabledError.Value))
        //    {
        //        return new MessagingEntityDisabledException(message, null);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.PublisherRevokedError.Value))
        //    {
        //        return new PublisherRevokedException(message, null);
        //    }
        //    else if (string.Equals(condition, AmqpErrorCode.Stolen.Value))
        //    {
        //        return new ReceiverDisconnectedException(message);
        //    }
        //    else if (string.Equals(condition, AmqpClientConstants.MessageNotFoundError.Value))
        //    {
        //        return new MessageNotFoundException(message);
        //    }
        //    else
        //    {
        //        return new MessagingException(message);
        //    }
        //}
    }
}
