//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#include "GameNetworkMessage.h"
#include "GameNetworkConnection.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameNetworkMessage, Log, All);


////////////////// FGameNetworkSerializeMessageTask //////////////////

void FGameNetworkSerializeMessageTask::DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	FBufferArchive Ar;
	Ar << Message->MessageType;
	Message->Serialize(Ar);

	// enqueue to recipients
	for (auto& Connection : RecipientConnections)
	{
		Connection->Send(Ar);
	}
}

ENamedThreads::Type FGameNetworkSerializeMessageTask::GetDesiredThread()
{
	return ENamedThreads::AnyThread;
}

TStatId FGameNetworkSerializeMessageTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FGameNetworkSerializeMessageTask, STATGROUP_TaskGraphTasks);
}

ESubsequentsMode::Type FGameNetworkSerializeMessageTask::GetSubsequentsMode()
{
	return ESubsequentsMode::FireAndForget;
}
