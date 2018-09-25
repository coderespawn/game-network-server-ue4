//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#pragma once

#include "CoreMinimal.h"
#include "Network/GameNetworkMessage.h"
#include "Cave/CaveChunkData.h"

namespace GameNetworkMessageTypes {
	static const FName SERVER_CHUNK_DATA = "ServerChunkData";
}


class PLATFORMARCHITECTRUNTIME_API FNetworkMessage_ServerChunkData : public FGameNetworkMessage {
public:
	FNetworkMessage_ServerChunkData();
	virtual bool Serialize(FArchive& Ar) override;

public:
	FName CaveSystemName;
	FCaveChunkData ChunkData;
};


class PLATFORMARCHITECTRUNTIME_API FPlanetExplorerNetworkMessageFactory : public IGameNetworkMessageFactory {
public:
	virtual FGameNetworkMessage* Create(FName MessageType) override;
};
