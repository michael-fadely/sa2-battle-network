#pragma once

#include "typedefs.h"
#include <map>
#include <sws/Packet.h>

/*
	Message types with the naming convention <group>_<START/END> are never sent;
	they're used to verify that a received message is within that group's range.

	Example:
	When receiving player messages, we check if the type is > P_START and < P_END.
*/

namespace nethax
{
	enum class Protocol
	{
		udp,
		tcp,
	};

	enum class MessageID : uint8_t
	{
		None,

		N_START,			// Marker: Start of Networking messages

		N_Sequence,			// UDP Sequence
		N_VersionCheck,		// Initial version check upon connection
		N_VersionMismatch,	// Client version mismatch
		N_VersionOK,		// Client version match
		N_Bind,				// UDP bind request/confirm
		N_Password,			// Server password
		N_PasswordMismatch,	// Incorrect server password
		N_Settings,			// Used for synchronizing settings
		N_Connected,		// Connection setup successful
		N_Ready,			// Client is ready at current sync block
		N_Disconnect,		// Request disconnect
		N_SetPlayer,		// Send a player number change
		N_PlayerNumber,		// The player number this message came from
		N_Node,				// The node number this message came from

		N_END,				// Marker: End of Networking messages
		I_START,			// Marker: Start of input messages

		I_Analog,
		I_AnalogAngle,
		I_Buttons,

		I_END,				// Marker: End of input messages
		M_START,			// Marker: Start of menu messages

		M_CostumeSelection,
		M_BattleSelection,
		M_BattleConfigSelection,
		M_CharacterChosen,
		M_CharacterSelection,
		M_StageSelection,

		M_END,				// Marker: End of menu messages
		P_START,			// Marker: Start of player messages

		P_Character,
		// CharObj1
		P_Action,
		P_NextAction,
		P_Status,
		P_Rotation,
		P_Position,
		P_Scale,
		// CharObj2
		P_Powerups,
		P_Upgrades,
		P_HP,
		P_Speed,
		P_Animation,
		// Sonic
		P_SpinTimer,
		P_Damage,
		P_DropRings,
		P_Kill,

		P_END,				// Marker: End of player messages
		S_START,			// Marker: Start of system messages

		S_FrameCount,
		S_KeepAlive,
		S_Seed,
		S_Win,
		S_Result,
		S_WinData,
		S_Stage,
		S_NextStage,
		S_RoundStart,
		S_2PReady,
		S_2PSpecials,
		S_BattleOptions,
		S_GameState,
		S_PauseSelection,
		S_Rings,
		S_Time,
		S_TimeStop,
		S_NBarrier,
		S_TBarrier,
		S_Speedup,
		S_Invincibility,
		S_ItemBoxItem,

		S_END,				// Marker: End of system messages
		Count
	};

	struct MessageStat
	{
		size_t tcp_count;
		size_t udp_count;
		ushort size;
	};

	const extern std::map<MessageID, const char*> MESSAGE_ID_STRING;

	void write_netstat_csv(std::ofstream& file, std::map<MessageID, MessageStat> map);
}

sws::Packet& operator <<(sws::Packet& packet, const nethax::MessageID& data);
sws::Packet& operator >>(sws::Packet& packet, nethax::MessageID& data);
