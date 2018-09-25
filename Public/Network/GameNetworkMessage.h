
#pragma once

#include "CoreMinimal.h"
#include "ArrayReader.h"

//////////////////////////// FGameNetworkSerializedMessage ////////////////////////////

class PLATFORMARCHITECTRUNTIME_API FGameNetworkMessage
{
public:
	virtual ~FGameNetworkMessage() {}

	virtual bool Serialize(FArchive& Ar) = 0;

	FName MessageType;
};

typedef TSharedPtr<class FGameNetworkMessage, ESPMode::ThreadSafe> FGameNetworkMessagePtr;
typedef TSharedRef<class FGameNetworkMessage, ESPMode::ThreadSafe> FGameNetworkMessageRef;

class PLATFORMARCHITECTRUNTIME_API IGameNetworkMessageFactory {
public:
	virtual ~IGameNetworkMessageFactory() {}
	virtual FGameNetworkMessage* Create(FName MessageType) = 0;
};
typedef TSharedPtr<class IGameNetworkMessageFactory> IGameNetworkMessageFactoryPtr;

//////////////////////////// IGameNetworkMessageHandler ////////////////////////////

class PLATFORMARCHITECTRUNTIME_API FGameNetworkConnection;

class IGameNetworkMessageHandler {
public:
	/**
	* Called by game network service when a new network node has been discovered.
	*
	* @param NodeId The identifier of the discovered network node.
	* @see ForgetTransportNode
	*/
	virtual void DiscoverTransportNode(const FGuid& NodeId) = 0;

	/**
	* Called by game network service when a network node has been lost.
	*
	* @param NodeId The identifier of the lost network node.
	* @see DiscoverTransportNode
	*/
	virtual void ForgetTransportNode(const FGuid& NodeId) = 0;

	/**
	* Called by game network service when a message was received.
	*
	* @param Context The context of the received message.
	* @param NodeId The identifier of the network node that received the message.
	*/
	virtual void ReceiveTransportMessage(const FGameNetworkMessageRef& Message, const FGuid& NodeId) = 0;


public:
	/** Virtual destructor. */
	virtual ~IGameNetworkMessageHandler() { }
};


////////////////// FGameNetworkSerializeMessageTask ////////////////// 

/**
* Implements an asynchronous task for serializing a message.
*/
class PLATFORMARCHITECTRUNTIME_API FGameNetworkSerializeMessageTask
{
public:

	/**
	* Creates and initializes a new instance.
	*
	* @param InMessageContext The context of the message to serialize.
	* @param InSerializedMessage Will hold the serialized message data.
	*/
	FGameNetworkSerializeMessageTask(FGameNetworkMessageRef InMessage, const TArray<TSharedPtr<FGameNetworkConnection>>& InRecipientConnections)
		: Message(InMessage)
		, RecipientConnections(InRecipientConnections)
	{ }

public:

	/**
	* Performs the actual task.
	*
	* @param CurrentThread The thread that this task is executing on.
	* @param MyCompletionGraphEvent The completion event.
	*/
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent);

	/**
	* Returns the name of the thread that this task should run on.
	*
	* @return Thread name.
	*/
	ENamedThreads::Type GetDesiredThread();

	/**
	* Gets the task's stats tracking identifier.
	*
	* @return Stats identifier.
	*/
	TStatId GetStatId() const;

	/**
	* Gets the mode for tracking subsequent tasks.
	*
	* @return Always track subsequent tasks.
	*/
	static ESubsequentsMode::Type GetSubsequentsMode();

private:

	/** Holds the context of the message to serialize. */
	FGameNetworkMessageRef Message;

	/** Connections we're going to enqueue the serialized message for */
	TArray<TSharedPtr<FGameNetworkConnection>> RecipientConnections;
};
