//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#include "GameNetwork.h"

#include "HAL/RunnableThread.h"
#include "Common/TcpSocketBuilder.h"
#include "Common/TcpListener.h"
#include "TaskGraphInterfaces.h"

#include "Network/GameNetworkConnection.h"
#include "Network/GameNetworkMessage.h"
#include "Network/GameNetworkPrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameNetwork, Log, All);

int32 FGameNetwork::DefaultServerPort = 3054;


FGameNetwork::FGameNetwork(const FIPv4Endpoint& InListenEndpoint, const TArray<FIPv4Endpoint>& InConnectToEndpoints, int32 InConnectionRetryDelay)
	: ListenEndpoint(InListenEndpoint)
	, ConnectToEndpoints(InConnectToEndpoints)
	, ConnectionRetryDelay(InConnectionRetryDelay)
	, bStopping(false)
	, SocketSubsystem(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	, Listener(nullptr)
{
	Thread = FRunnableThread::Create(this, TEXT("FGameNetwork"), 128 * 1024, TPri_Normal);
}


FGameNetwork::~FGameNetwork()
{
	if (Thread != nullptr)
	{
		Thread->Kill(true);
		delete Thread;
	}

	StopService();
}


/* IMessageTransport interface
*****************************************************************************/

bool FGameNetwork::StartService(TSharedPtr<IGameNetworkMessageHandler> InMessageHandler, IGameNetworkMessageFactoryPtr InMessageFactory)
{
	MessageHandler = InMessageHandler;
	MessageFactory = InMessageFactory;

	if (ListenEndpoint != FIPv4Endpoint::Any)
	{
		Listener = new FTcpListener(ListenEndpoint);
		Listener->OnConnectionAccepted().BindRaw(this, &FGameNetwork::HandleListenerConnectionAccepted);
	}

	// outgoing connections
	for (auto& ConnectToEndPoint : ConnectToEndpoints)
	{
		AddOutgoingConnection(ConnectToEndPoint);
	}

	return true;
}

void FGameNetwork::AddOutgoingConnection(const FIPv4Endpoint& Endpoint)
{
	FSocket* Socket = FTcpSocketBuilder(TEXT("FGameNetwork.RemoteConnection"));

	if (Socket == nullptr)
	{
		return;
	}

	if (!Socket->Connect(*Endpoint.ToInternetAddr()))
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
	else
	{
		PendingConnections.Enqueue(MakeShareable(new FGameNetworkConnection(Socket, Endpoint, ConnectionRetryDelay, MessageFactory)));
	}
}

void FGameNetwork::RemoveOutgoingConnection(const FIPv4Endpoint& Endpoint)
{
	ConnectionEndpointsToRemove.Enqueue(Endpoint);
}

void FGameNetwork::StopService()
{
	bStopping = true;

	if (Listener)
	{
		delete Listener;
		Listener = nullptr;
	}

	for (auto& Connection : Connections)
	{
		Connection->Close();
	}

	Connections.Empty();
	PendingConnections.Empty();
	ConnectionEndpointsToRemove.Empty();
}


bool FGameNetwork::SendMessage(const FGameNetworkMessageRef& Message, const TArray<FGuid>& Recipients)
{
	// Handle any queued changes to the NodeConnectionMap
	FGameNodeConnectionMapUpdate UpdateInfo;
	while (NodeConnectionMapUpdates.Dequeue(UpdateInfo))
	{
		check(UpdateInfo.NodeId.IsValid());
		if (UpdateInfo.bNewNode)
		{
			TSharedPtr<FGameNetworkConnection> ConnectionPtr = UpdateInfo.Connection.Pin();
			if (ConnectionPtr.IsValid())
			{
				NodeConnectionMap.Add(UpdateInfo.NodeId, ConnectionPtr);
			}
		}
		else
		{
			NodeConnectionMap.Remove(UpdateInfo.NodeId);
		}
	}

	// Work out which connections we need to send this message to.
	TArray<TSharedPtr<FGameNetworkConnection>> RecipientConnections;

	if (Recipients.Num() == 0)
	{
		// broadcast the message to all valid connections
		RecipientConnections = Connections.FilterByPredicate([&](const TSharedPtr<FGameNetworkConnection>& Connection) -> bool
		{
			return Connection->GetConnectionState() == FGameNetworkConnection::STATE_Connected;
		});
	}
	else
	{
		// Find connections for each recipient.  We do not transport unicast messages for unknown nodes.
		for (auto& Recipient : Recipients)
		{
			TSharedPtr<FGameNetworkConnection>* RecipientConnection = NodeConnectionMap.Find(Recipient);
			if (RecipientConnection && (*RecipientConnection)->GetConnectionState() == FGameNetworkConnection::STATE_Connected)
			{
				RecipientConnections.AddUnique(*RecipientConnection);
			}
		}
	}

	if (RecipientConnections.Num() == 0)
	{
		return false;
	}

	UE_LOG(LogGameNetwork, Verbose, TEXT("Transporting message to %d connections"), RecipientConnections.Num());

	TGraphTask<FGameNetworkSerializeMessageTask>::CreateTask().ConstructAndDispatchWhenReady(Message, RecipientConnections);

	return true;
}


/* FRunnable interface
*****************************************************************************/

void FGameNetwork::Exit()
{
	// do nothing
}


bool FGameNetwork::Init()
{
	return true;
}


uint32 FGameNetwork::Run()
{
	while (!bStopping)
	{
		// new connections
		{
			TSharedPtr<FGameNetworkConnection> Connection;
			while (PendingConnections.Dequeue(Connection))
			{
				Connection->OnConnectionStateChanged().BindRaw(this, &FGameNetwork::HandleConnectionStateChanged, Connection);
				Connection->Start();
				Connections.Add(Connection);
			}
		}

		// connections to remove
		{
			FIPv4Endpoint Endpoint;
			while (ConnectionEndpointsToRemove.Dequeue(Endpoint))
			{
				for (int32 Index = 0; Index < Connections.Num(); Index++)
				{
					auto& Connection = Connections[Index];

					if (Connection->GetRemoteEndpoint() == Endpoint)
					{
						Connection->Close();
						break;
					}
				}
			}
		}

		int32 ActiveConnections = 0;
		for (int32 Index = 0; Index < Connections.Num(); Index++)
		{
			auto& Connection = Connections[Index];

			// handle disconnected by remote
			switch (Connection->GetConnectionState())
			{
			case FGameNetworkConnection::STATE_Connected:
				ActiveConnections++;
				break;

			case FGameNetworkConnection::STATE_Disconnected:
				Connections.RemoveAtSwap(Index);
				Index--;
				break;

			default:
				break;
			}
		}

		// incoming messages
		{
			for (auto& Connection : Connections)
			{
				FGameNetworkMessagePtr Message;
				FGuid SenderNodeId;

				while (Connection->Receive(Message, SenderNodeId))
				{
					UE_LOG(LogGameNetwork, Verbose, TEXT("Received message of type: %s"), *Message->MessageType.ToString());
					MessageHandler->ReceiveTransportMessage(Message.ToSharedRef(), SenderNodeId);
				}
			}
		}

		FPlatformProcess::Sleep(ActiveConnections > 0 ? 0.01f : 1.f);
	}

	return 0;
}


void FGameNetwork::Stop()
{
	bStopping = true;
}


/* FGameNetwork callbacks
*****************************************************************************/

bool FGameNetwork::HandleListenerConnectionAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
	PendingConnections.Enqueue(MakeShareable(new FGameNetworkConnection(ClientSocket, ClientEndpoint, 0, MessageFactory)));

	return true;
}


void FGameNetwork::HandleConnectionStateChanged(TSharedPtr<FGameNetworkConnection> Connection)
{
	const FGuid NodeId = Connection->GetRemoteNodeId();
	const FIPv4Endpoint RemoteEndpoint = Connection->GetRemoteEndpoint();
	const FGameNetworkConnection::EConnectionState State = Connection->GetConnectionState();

	if (State == FGameNetworkConnection::STATE_Connected)
	{
		NodeConnectionMapUpdates.Enqueue(FGameNodeConnectionMapUpdate(true, NodeId, TWeakPtr<FGameNetworkConnection>(Connection)));
		MessageHandler->DiscoverTransportNode(NodeId);

		UE_LOG(LogGameNetwork, Verbose, TEXT("Discovered node '%s' on connection '%s'..."), *NodeId.ToString(), *RemoteEndpoint.ToString());
	}
	else if (NodeId.IsValid())
	{
		UE_LOG(LogGameNetwork, Verbose, TEXT("Lost node '%s' on connection '%s'..."), *NodeId.ToString(), *RemoteEndpoint.ToString());

		NodeConnectionMapUpdates.Enqueue(FGameNodeConnectionMapUpdate(false, NodeId, TWeakPtr<FGameNetworkConnection>(Connection)));
		MessageHandler->ForgetTransportNode(NodeId);
	}
}
