//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#pragma once

#include "CoreMinimal.h"
#include "Networking.h"
#include "Runnable.h"

class FSocket;
class FTcpListener;
class FGameNetworkConnection;
class IGameNetworkMessageHandler;
typedef TSharedPtr<class IGameNetworkMessageFactory> IGameNetworkMessageFactoryPtr;
typedef TSharedPtr<class FGameNetworkMessage, ESPMode::ThreadSafe> FGameNetworkMessagePtr;
typedef TSharedRef<class FGameNetworkMessage, ESPMode::ThreadSafe> FGameNetworkMessageRef;


/** Entry specifying addition or removal to/from the NodeConnectionMap */
struct PLATFORMARCHITECTRUNTIME_API FGameNodeConnectionMapUpdate
{
	FGameNodeConnectionMapUpdate(bool bInNewNode, const FGuid& InNodeId, const TWeakPtr<FGameNetworkConnection>& InConnection)
		: bNewNode(bInNewNode)
		, NodeId(InNodeId)
		, Connection(InConnection)
	{ }

	FGameNodeConnectionMapUpdate()
		: bNewNode(false)
	{ }

	bool bNewNode;
	FGuid NodeId;
	TWeakPtr<FGameNetworkConnection> Connection;
};

class PLATFORMARCHITECTRUNTIME_API FGameNetwork
	: FRunnable
{
public:

	/**
	* Creates and initializes a new instance.
	*
	*/
	FGameNetwork(const FIPv4Endpoint& InListenEndpoint, const TArray<FIPv4Endpoint>& InConnectToEndpoints, int32 InConnectionRetryDelay);

	/** Virtual destructor. */
	virtual ~FGameNetwork();

	static TSharedPtr<FGameNetwork> CreateServer(const FIPv4Endpoint& InServerEndpoint, int32 InConnectionRetryDelay) {
		return MakeShareable(new FGameNetwork(InServerEndpoint, TArray<FIPv4Endpoint>(), InConnectionRetryDelay));
	}
	static TSharedPtr<FGameNetwork> CreateClient(const FIPv4Endpoint& InConnectToEndpoint, int32 InConnectionRetryDelay) {
		TArray<FIPv4Endpoint> EndPoints;
		EndPoints.Add(InConnectToEndpoint);
		return MakeShareable(new FGameNetwork(FIPv4Endpoint::Any, EndPoints, InConnectionRetryDelay));
	}

public:

	void AddOutgoingConnection(const FIPv4Endpoint& Endpoint);
	void RemoveOutgoingConnection(const FIPv4Endpoint& Endpoint);


public:

	//~ IMessageTransport interface

	FName GetDebugName() const { return "GameNetworkService"; }

	bool StartService(TSharedPtr<IGameNetworkMessageHandler> InMessageHandler, IGameNetworkMessageFactoryPtr InMessageFactory);
	void StopService();
	bool SendMessage(const FGameNetworkMessageRef& Message, const TArray<FGuid>& Recipients);

public:

	//~ FRunnable interface

	virtual void Exit() override;
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

private:

	/** Callback for accepted connections to the local server. */
	bool HandleListenerConnectionAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint);

	/** Callback from connections for node discovery/loss */
	void HandleConnectionStateChanged(TSharedPtr<FGameNetworkConnection> Connection);

private:

	/** Current settings */
	FIPv4Endpoint ListenEndpoint;
	TArray<FIPv4Endpoint> ConnectToEndpoints;
	int32 ConnectionRetryDelay;

	/** For the thread */
	bool bStopping;

	/** Holds a pointer to the socket sub-system. */
	ISocketSubsystem* SocketSubsystem;

	/** Holds the local listener for incoming tunnel connections. */
	FTcpListener* Listener;

	/** Current connections */
	TArray<TSharedPtr<FGameNetworkConnection>> Connections;

	/** Map nodes to connections. We do not transport unicast messages for unknown nodes. */
	TMap<FGuid, TSharedPtr<FGameNetworkConnection>> NodeConnectionMap;

	/** Holds a queue of changes to NodeConnectionMap. */
	TQueue<FGameNodeConnectionMapUpdate, EQueueMode::Mpsc> NodeConnectionMapUpdates;

	/** Holds a queue of pending connections. */
	TQueue<TSharedPtr<FGameNetworkConnection>, EQueueMode::Mpsc> PendingConnections;

	/** Queue of end-points describing connection we want to remove */
	TQueue<FIPv4Endpoint, EQueueMode::Mpsc> ConnectionEndpointsToRemove;

	/** Holds the thread object. */
	FRunnableThread* Thread;

	/** Message transport handler. */
	TSharedPtr<IGameNetworkMessageHandler> MessageHandler;
	
	IGameNetworkMessageFactoryPtr MessageFactory;

public:
	static int32 DefaultServerPort;
};
