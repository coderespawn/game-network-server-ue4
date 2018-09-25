//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#pragma once

#include "CoreMinimal.h"
#include "Network/GameNetworkMessage.h"
#include "Cave/CaveChunkDataTransport.h"

class APlatformCaveSystem;
typedef TSharedPtr<class FGameNetworkMessage, ESPMode::ThreadSafe> FGameNetworkMessagePtr;

DECLARE_DELEGATE_OneParam(FOnClientConnectionChanged, const FGuid&);

class PLATFORMARCHITECTRUNTIME_API FPlanetExplorerServerMessageHandler : public IGameNetworkMessageHandler {
public:
	virtual void DiscoverTransportNode(const FGuid& NodeId) override;

	virtual void ForgetTransportNode(const FGuid& NodeId);

	virtual void ReceiveTransportMessage(const FGameNetworkMessageRef& Message, const FGuid& NodeId);

	void CreateTransportChunkMessages(const FGuid& InClientID, APlatformCaveSystem* CaveSystem, TArray<FGameNetworkMessagePtr>& OutMessagesToSend);

	TArray<FGuid> DiscoveredNodes;

	FOnClientConnectionChanged& GetOnClientConnected() { return OnClientConnected; }
	FOnClientConnectionChanged& GetOnClientDisconnected() { return OnClientDisconnected; }

private:
	FCaveChunkDataTransport<FGuid> DataTransportDiff;
	FOnClientConnectionChanged OnClientConnected;
	FOnClientConnectionChanged OnClientDisconnected;
};

DECLARE_DELEGATE_OneParam(FPlanetExplorerClientMessageRecievedDelegate, const FGameNetworkMessageRef&);

class PLATFORMARCHITECTRUNTIME_API FPlanetExplorerClientMessageHandler : public IGameNetworkMessageHandler {
public:
	virtual void DiscoverTransportNode(const FGuid& NodeId) override;

	virtual void ForgetTransportNode(const FGuid& NodeId);

	virtual void ReceiveTransportMessage(const FGameNetworkMessageRef& Message, const FGuid& NodeId);

	FPlanetExplorerClientMessageRecievedDelegate& GetOnMessageReceived() { return OnMessageReceived; }
	
public:
	TArray<FGuid> DiscoveredNodes;

private:
	FPlanetExplorerClientMessageRecievedDelegate OnMessageReceived;
};

