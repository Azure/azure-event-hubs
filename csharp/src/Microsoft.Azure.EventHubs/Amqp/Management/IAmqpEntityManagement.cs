namespace Microsoft.Azure.EventHubs.Amqp.Management
{
    using System;
    using System.Threading.Tasks;

    [Management]
    public interface IAmqpEntityManagement
    {
        [ManagementOperation(Name = AmqpClientConstants.ReadOperationValue, AsyncPattern = AsyncPattern.Task)]
        Task<T> GetRuntimeInfoAsync<T>(
            [ManagementParam(Name = AmqpClientConstants.ManagementEntityTypeKey, Location = ManagementParamLocation.ApplicationProperties)] string entityType,
            [ManagementParam(Name = AmqpClientConstants.EntityNameKey, Location = ManagementParamLocation.ApplicationProperties)] string path,
            [ManagementParam(Name = AmqpClientConstants.PartitionNameKey, Location = ManagementParamLocation.ApplicationProperties)] string partitionId,
            [ManagementParam(Name = AmqpClientConstants.ManagementSecurityTokenKey, Location = ManagementParamLocation.ApplicationProperties)] string securityToken,
            TimeSpan timeout);
    }
}