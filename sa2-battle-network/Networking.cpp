#include "stdafx.h"

#include <fstream>
#include <map>

#include <sws/Packet.h>

#include "Networking.h"

using namespace nethax;

void nethax::write_netstat_csv(std::ofstream& file, std::map<MessageID, MessageStat> map)
{
	file << "Message ID,TCP count,UDP count,Total count,Size,TCP size,UDP size,Total size" << std::endl;
	for (const auto& i : map)
	{
		const MessageStat& stat = i.second;

		const auto total_count = stat.tcp_count + stat.udp_count;
		const auto tcp_bytes   = stat.size * stat.tcp_count;
		const auto udp_bytes   = stat.size * stat.udp_count;
		const auto bytes       = stat.size * total_count;

		file << static_cast<ushort>(i.first) << " - "
			<< MESSAGE_ID_STRING.at(i.first) << ',' // Message ID
			<< stat.tcp_count << ','                // TCP count
			<< stat.udp_count << ','                // UDP count
			<< total_count << ','                   // Total count
			<< stat.size << ','                     // Size
			<< tcp_bytes << ','                     // TCP size
			<< udp_bytes << ','                     // UDP size
			<< bytes                                // Total size
			<< std::endl;
	}
}

// Regex magic: ^(.+),(?:(\s*//.+)?)$
// to:          { MessageID::$1, "$1" },

const std::unordered_map<MessageID, const char*> nethax::MESSAGE_ID_STRING = {
	{ MessageID::None, "None" },

	{ MessageID::N_START, "N_START" },

	{ MessageID::N_VersionCheck, "N_VersionCheck" },
	{ MessageID::N_VersionMismatch, "N_VersionMismatch" },
	{ MessageID::N_VersionOK, "N_VersionOK" },
	{ MessageID::N_Password, "N_Password" },
	{ MessageID::N_PasswordMismatch, "N_PasswordMismatch" },
	{ MessageID::N_Settings, "N_Settings" },
	{ MessageID::N_Connected, "N_Connected" },
	{ MessageID::N_Ready, "N_Ready" },
	{ MessageID::N_Disconnect, "N_Disconnect" },
	{ MessageID::N_SetPlayerNumber, "N_SetPlayerNumber" },
	{ MessageID::N_PlayerNumber, "N_PlayerNumber" },
	{ MessageID::N_Node, "N_Node" },

	{ MessageID::N_END, "N_END" },
	{ MessageID::I_START, "I_START" },

	{ MessageID::I_Analog, "I_Analog" },
	{ MessageID::I_AnalogAngle, "I_AnalogAngle" },
	{ MessageID::I_Buttons, "I_Buttons" },

	{ MessageID::I_END, "I_END" },
	{ MessageID::M_START, "M_START" },

	{ MessageID::M_CostumeSelection, "M_CostumeSelection" },
	{ MessageID::M_BattleSelection, "M_BattleSelection" },
	{ MessageID::M_BattleConfigSelection, "M_BattleConfigSelection" },
	{ MessageID::M_CharacterChosen, "M_CharacterChosen" },
	{ MessageID::M_CharacterSelection, "M_CharacterSelection" },
	{ MessageID::M_StageSelection, "M_StageSelection" },

	{ MessageID::M_END, "M_END" },
	{ MessageID::P_START, "P_START" },

	{ MessageID::P_Character, "P_Character" },
	{ MessageID::P_Action, "P_Action" },
	{ MessageID::P_NextAction, "P_NextAction" },
	{ MessageID::P_Status, "P_Status" },
	{ MessageID::P_Rotation, "P_Rotation" },
	{ MessageID::P_Position, "P_Position" },
	{ MessageID::P_Scale, "P_Scale" },
	{ MessageID::P_Powerups, "P_Powerups" },
	{ MessageID::P_Upgrades, "P_Upgrades" },
	{ MessageID::P_HP, "P_HP" },
	{ MessageID::P_Speed, "P_Speed" },
	{ MessageID::P_Animation, "P_Animation" },
	{ MessageID::P_SpinTimer, "P_SpinTimer" },
	{ MessageID::P_Damage, "P_Damage" },
	{ MessageID::P_DropRings, "P_DropRings" },
	{ MessageID::P_Kill, "P_Kill" },

	{ MessageID::P_END, "P_END" },
	{ MessageID::S_START, "S_START" },

	{ MessageID::S_KeepAlive, "S_KeepAlive" },
	{ MessageID::S_Seed, "S_Seed" },
	{ MessageID::S_Win, "S_Win" },
	{ MessageID::S_Result, "S_Result" },
	{ MessageID::S_WinData, "S_WinData" },
	{ MessageID::S_Stage, "S_Stage" },
	{ MessageID::S_NextStage, "S_NextStage" },
	{ MessageID::S_RoundStart, "S_RoundStart" },
	{ MessageID::S_2PReady, "S_2PReady" },
	{ MessageID::S_2PSpecials, "S_2PSpecials" },
	{ MessageID::S_BattleOptions, "S_BattleOptions" },
	{ MessageID::S_GameState, "S_GameState" },
	{ MessageID::S_PauseSelection, "S_PauseSelection" },
	{ MessageID::S_Rings, "S_Rings" },
	{ MessageID::S_Time, "S_Time" },
	{ MessageID::S_TimeStop, "S_TimeStop" },
	{ MessageID::S_NBarrier, "S_NBarrier" },
	{ MessageID::S_TBarrier, "S_TBarrier" },
	{ MessageID::S_Speedup, "S_Speedup" },
	{ MessageID::S_Invincibility, "S_Invincibility" },
	{ MessageID::S_ItemBoxItem, "S_ItemBoxItem" },

	{ MessageID::S_END, "S_END" }
};

sws::Packet& operator<<(sws::Packet& packet, const MessageID& data)
{
	return packet << static_cast<uint8_t>(data);
}

sws::Packet& operator>>(sws::Packet& packet, MessageID& data)
{
	uint8_t d;
	packet >> d;
	data = static_cast<MessageID>(d);
	return packet;
}
