//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#include "Network/Messages/PlanetExplorerNetworkMessages.h"

//////////////////////////// FNetworkMessageChunkData ////////////////////////////
FNetworkMessage_ServerChunkData::FNetworkMessage_ServerChunkData()
{
	MessageType = GameNetworkMessageTypes::SERVER_CHUNK_DATA;
}

bool FNetworkMessage_ServerChunkData::Serialize(FArchive& Ar)
{
	Ar << CaveSystemName;
	Ar << ChunkData.WorldLocation;
	Ar << ChunkData.WorldSize;
	Ar << ChunkData.DensityData;
	Ar << ChunkData.MaterialData;
	Ar << ChunkData.DensityWidth;
	Ar << ChunkData.DensityHeight;

	return true;
}


//////////////////////////// FPlanetExplorerNetworkMessageFactor ////////////////////////////
FGameNetworkMessage* FPlanetExplorerNetworkMessageFactory::Create(FName MessageType)
{
	if (MessageType == GameNetworkMessageTypes::SERVER_CHUNK_DATA) return new FNetworkMessage_ServerChunkData();

	return nullptr;
}
