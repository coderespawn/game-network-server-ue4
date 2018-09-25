//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#include "Network/Messages/PlanetExplorerNetworkMessageHandler.h"
#include "Cave/CaveSystemActor.h"
#include "Cave/CaveSystemChunk.h"
#include "PlanetExplorerNetworkMessages.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameMessageHandler, Log, All);

/////////////////////////// FPlanetExplorerServerMessageHandler /////////////////////////// 

void FPlanetExplorerServerMessageHandler::DiscoverTransportNode(const FGuid& ClientId)
{
	DiscoveredNodes.AddUnique(ClientId);
	UE_LOG(LogGameMessageHandler, Log, TEXT("[Server] Discovered node: %s"), *ClientId.ToString());

	OnClientConnected.ExecuteIfBound(ClientId);

	DataTransportDiff.AddClient(ClientId);
}

void FPlanetExplorerServerMessageHandler::ForgetTransportNode(const FGuid& ClientId)
{
	DiscoveredNodes.Remove(ClientId);
	UE_LOG(LogGameMessageHandler, Log, TEXT("[Server] Forgotten node: %s"), *ClientId.ToString());
	
	OnClientDisconnected.ExecuteIfBound(ClientId);

	DataTransportDiff.RemoveClient(ClientId);
}

void FPlanetExplorerServerMessageHandler::ReceiveTransportMessage(const FGameNetworkMessageRef& Message, const FGuid& NodeId)
{
	UE_LOG(LogGameMessageHandler, Log, TEXT("[Server] Recieved message from node: %s, Message Type [%s]"), *NodeId.ToString(), *Message->MessageType.ToString());
}

void FPlanetExplorerServerMessageHandler::CreateTransportChunkMessages(const FGuid& InClientID, APlatformCaveSystem* CaveSystem, TArray<FGameNetworkMessagePtr>& OutMessagesToSend)
{
	TArray<FCaveChunkDataTransportItem> Items;
	DataTransportDiff.CreateMessages(InClientID, CaveSystem, Items);
	for (const FCaveChunkDataTransportItem& Item : Items) {
		// TODO: Optimize me.  too many copies
		TSharedPtr<FNetworkMessage_ServerChunkData, ESPMode::ThreadSafe> Message = MakeShareable(new FNetworkMessage_ServerChunkData);
		Message->CaveSystemName = Item.MapName;
		Message->ChunkData = *Item.ChunkData;
		OutMessagesToSend.Add(Message);
	}
}

/////////////////////////// FPlanetExplorerClientMessageHandler /////////////////////////// 

void FPlanetExplorerClientMessageHandler::DiscoverTransportNode(const FGuid& NodeId)
{
	DiscoveredNodes.Add(NodeId);
	UE_LOG(LogGameMessageHandler, Log, TEXT("[Client] Discovered node: %s"), *NodeId.ToString());
}

void FPlanetExplorerClientMessageHandler::ForgetTransportNode(const FGuid& NodeId)
{
	DiscoveredNodes.Remove(NodeId);
	UE_LOG(LogGameMessageHandler, Log, TEXT("[Client] Forgotten node: %s"), *NodeId.ToString());
}

void FPlanetExplorerClientMessageHandler::ReceiveTransportMessage(const FGameNetworkMessageRef& Message, const FGuid& NodeId)
{
	UE_LOG(LogGameMessageHandler, Log, TEXT("[Client] Recieved node [%s], Message Type [%s]"), *NodeId.ToString(), *Message->MessageType.ToString());
	OnMessageReceived.ExecuteIfBound(Message);
}
