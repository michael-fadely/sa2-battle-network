#include "stdafx.h"
#include <fstream>
#include <map>
#include "Networking.h"

using namespace nethax;

void nethax::WriteNetStatCSV(std::ofstream& file, std::map<MessageID, MessageStat> map)
{
	file << "Message ID,TCP count,UDP count,Total count,Size (bytes),TCP bytes,UDP bytes,Total bytes" << std::endl;
	for (auto& i : map)
	{
		uint total = i.second.tcpCount + i.second.udpCount;
		uint bytes = i.second.size * total;
		uint tcp_bytes = i.second.size * i.second.tcpCount;
		uint udp_bytes = i.second.size * i.second.udpCount;

		file << (ushort)i.first << " - " << MessageID_string.at(i.first) << ','
			<< i.second.tcpCount << ','
			<< i.second.udpCount << ','
			<< total << ','
			<< i.second.size << ','
			<< tcp_bytes << ','
			<< udp_bytes << ','
			<< bytes
			<< std::endl;
	}
}


// Regex magic: ^(.+),*$
// to: { MessageID::$1, "$1" },

const std::map<MessageID, const char*> nethax::MessageID_string = {
	{ MessageID::None, "None" },

	{ MessageID::N_START, "N_START" },

	{ MessageID::N_VersionCheck, "N_VersionCheck" },
	{ MessageID::N_VersionMismatch, "N_VersionMismatch" },
	{ MessageID::N_VersionOK, "N_VersionOK" },
	{ MessageID::N_Bind, "N_Bind" },
	{ MessageID::N_Settings, "N_Settings" },
	{ MessageID::N_Connected, "N_Connected" },
	{ MessageID::N_Ready, "N_Ready" },
	{ MessageID::N_Disconnect, "N_Disconnect" },

	{ MessageID::N_END, "N_END" },
	{ MessageID::I_START, "I_START" },

	{ MessageID::I_Analog, "I_Analog" },
	{ MessageID::I_AnalogThing, "I_AnalogThing" },
	{ MessageID::I_Buttons, "I_Buttons" },

	{ MessageID::I_END, "I_END" },
	{ MessageID::M_START, "M_START" },

	{ MessageID::M_AltCharacter, "M_AltCharacter" },
	{ MessageID::M_BattleSelection, "M_BattleSelection" },
	{ MessageID::M_BattleConfigSelection, "M_BattleConfigSelection" },
	{ MessageID::M_CharacterChosen, "M_CharacterChosen" },
	{ MessageID::M_CharacterSelection, "M_CharacterSelection" },
	{ MessageID::M_StageSelection, "M_StageSelection" },

	{ MessageID::M_END, "M_END" },
	{ MessageID::P_START, "P_START" },


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
	{ MessageID::P_Hurt, "P_Hurt" },
	{ MessageID::P_Kill, "P_Kill" },

	{ MessageID::P_END, "P_END" },
	{ MessageID::S_START, "S_START" },

	{ MessageID::S_FrameCount, "S_FrameCount" },
	{ MessageID::S_KeepAlive, "S_KeepAlive" },
	{ MessageID::S_Seed, "S_Seed" },
	{ MessageID::S_Stage, "S_Stage" },
	{ MessageID::S_2PReady, "S_2PReady" },
	{ MessageID::S_2PSpecials, "S_2PSpecials" },
	{ MessageID::S_BattleOptions, "S_BattleOptions" },
	{ MessageID::S_GameState, "S_GameState" },
	{ MessageID::S_PauseSelection, "S_PauseSelection" },
	{ MessageID::S_Rings, "S_Rings" },
	{ MessageID::S_Time, "S_Time" },
	{ MessageID::S_TimeStop, "S_TimeStop" },

	{ MessageID::S_END, "S_END" }
};
