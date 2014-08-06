// Standard Includes
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Global Includes
#include <LazyTypedefs.h>

// Local Includes
#include "Globals.h"
#include "Common.h"
#include "CommonEnums.h"

#include "Networking.h"
#include "PacketExtensions.h"
#include "AdventurePacketOverloads.h"
#include "PacketHandler.h"

#include "SA2ModLoader.h"
#include "ModLoaderExtensions.h"
#include "AddressList.h"
#include "ActionBlacklist.h"


// This Class
#include "MemoryHandler.h"

using namespace std;
using namespace sf;
using namespace sa2bn;

const uint rotatemargin = ((float)11.25 * (float)(65536 / 360));
uint rotateTimer = 0;
inline const bool RotationMargin(const Rotation& last, const Rotation& current)
{
	return ((max(last.x, current.x) - min(last.x, current.x)) >= rotatemargin
		|| (max(last.y, current.y) - min(last.y, current.y)) >= rotatemargin
		|| (max(last.z, current.z) - min(last.z, current.z)) >= rotatemargin
		|| memcmp(&last, &current, sizeof(Rotation)) != 0 && Duration(rotateTimer) >= 125);
}

const float speedmargin = 0.25;
uint speedTimer = 0;
const bool SpeedMargin(const float last, const float current)
{
	return ((max(last, current) - min(last, current)) >= speedmargin
		|| last != current && current < speedmargin
		|| last != current && Duration(speedTimer) >= 125);
}


/*
//	Memory Handler Class
*/

MemoryHandler::MemoryHandler()
{
	cAt2PMenu[0] = false;
	cAt2PMenu[1] = false;
	lAt2PMenu[0] = false;
	lAt2PMenu[1] = false;

	firstMenuEntry = false;
	wroteP2Start = false;
	splitToggled = false;
	Teleported = false;
	writePlayer = false;

	local = {};
	recvInput = {};
	sendInput = {};

	thisFrame = 0;
	lastFrame = 0;

	return;
}
MemoryHandler::~MemoryHandler()
{
}

void MemoryHandler::ReceiveSafe(sf::Packet& packet)
{
	cout << ">> ";
	if (Globals::Networking.recvSafe(packet) == sf::Socket::Status::Done)
	{
		uchar id;
		packet >> id;
		while (!packet.endOfPacket())
		{
			switch (id)
			{
			case MSG_NULL:
				cout << "Reached end of packet." << endl;
				break;

			case MSG_COUNT:
				cout << "Received message count?! Malformed packet warning!" << endl;
				break;

			case MSG_DISCONNECT:
				cout << "Received disconnect request from client." << endl;
				Globals::Networking.Disconnect(true);

			default:
				ReceiveSystem(id, packet);
				ReceiveInput(id, packet);
				ReceivePlayer(id, packet);
				ReceiveMenu(id, packet);
			}
		}
	}
}
void MemoryHandler::ReceiveFast(sf::Packet& packet)
{
	cout << ">> ";
	if (Globals::Networking.recvFast(packet) == sf::Socket::Status::Done)
	{
		uchar id;
		packet >> id;
		while (!packet.endOfPacket())
		{
			switch (id)
			{
			case MSG_NULL:
				cout << "Reached end of packet." << endl;
				break;

			case MSG_COUNT:
				cout << "Received message count?! Malformed packet warning!" << endl;
				break;

			case MSG_DISCONNECT:
				cout << "Received disconnect request from client." << endl;
				Globals::Networking.Disconnect(true);

			default:
				ReceiveSystem(id, packet);
				ReceiveInput(id, packet);
				ReceivePlayer(id, packet);
				ReceiveMenu(id, packet);
			}
		}
	}
}

void MemoryHandler::RecvLoop()
{
	GetFrame();
	if (Globals::Networking.isConnected())
	{
		if (CurrentMenu[0] < Menu::BATTLE)
			Globals::Networking.Disconnect();

		sf::Packet packet;
		ReceiveFast(packet);
		ReceiveSafe(packet);
	}
	SetFrame();
}

void MemoryHandler::SendLoop()
{
	GetFrame();

	SendInput();
	SendSystem();
	SendPlayer();
	SendMenu();

	SetFrame();
}

void MemoryHandler::SendSystem()
{
	PacketEx safe(true), fast(false);

	if (GameState >= GameState::LOAD_FINISHED && TwoPlayerMode > 0)
	{
		if (local.system.GameState != GameState && GameState > GameState::LOAD_FINISHED)
		{
			if (safe.addType(MSG_S_GAMESTATE))
			{
				cout << "<< Sending gamestate [" << (ushort)GameState << ']' << endl;

				safe << GameState;
				local.system.GameState = GameState;
			}
		}

		if (local.system.PauseSelection != PauseSelection)
		{
			if (safe.addType(MSG_S_PAUSESEL))
			{
				safe << PauseSelection;
				local.system.PauseSelection = PauseSelection;
			}
		}

		if (local.game.TimerSeconds != TimerSeconds && Globals::Networking.isServer())
		{
			if (fast.addType(MSG_S_TIME))
			{
				fast << TimerMinutes << TimerSeconds << TimerFrames;
				memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
			}
		}

		if (local.game.TimeStopMode != TimeStopMode)
		{
			if (safe.addType(MSG_S_TIMESTOP))
			{
				cout << "<< Sending time stop [";
				// Swap the value since player 1 is relative to the client

				switch (TimeStopMode)
				{
				default:
				case 0:
					cout << 0;
					safe << (char)0;
					break;

				case 1:
					cout << 2;
					safe << (char)2;
					break;

				case 2:
					cout << 1;
					safe << (char)1;
					break;
				}

				cout << ']' << endl;
				local.game.TimeStopMode = TimeStopMode;
			}
		}
	}
	/*
	if (GameState != GameState::INGAME || TimeStopMode > 0 || !Globals::Networking.isServer())
	{
	if (Duration(packetHandler->getSentKeepalive()) >= 1000)
	{
	Socket->writeByte(MSG_NULL); Socket->writeByte(1);
	Socket->writeByte(MSG_KEEPALIVE);

	packetHandler->SendMsg();
	packetHandler->setSentKeepalive();
	}
	}
	*/

	Globals::Networking.Send(fast);
	Globals::Networking.Send(safe);
}
void MemoryHandler::SendInput(/*uint sendTimer*/)
{
	PacketEx safe(true), fast(false);

	if (CurrentMenu[0] == 16 || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		if (!CheckFrame())
			ToggleSplitscreen();

		if (sendInput.HeldButtons != ControllersRaw[0].HeldButtons)
		{
			if (safe.addType(MSG_I_BUTTONS))
			{
				safe << ControllersRaw[0].HeldButtons;
				sendInput.HeldButtons = ControllersRaw[0].HeldButtons;
			}
		}

		if (sendInput.LeftStickX != ControllersRaw[0].LeftStickX || sendInput.LeftStickY != ControllersRaw[0].LeftStickY)
		{
			if (GameState == GameState::INGAME)
			{
				/*if (Duration(analogTimer) >= 125)
				{*/
				if (ControllersRaw[0].LeftStickX == 0 && ControllersRaw[0].LeftStickY == 0)
				{
					if (safe.addType(MSG_I_ANALOG))
					{
						safe << ControllersRaw[0].LeftStickX << ControllersRaw[0].LeftStickY;
						sendInput.LeftStickX = ControllersRaw[0].LeftStickX;
						sendInput.LeftStickY = ControllersRaw[0].LeftStickY;
					}
				}
				else
				{
					if (fast.addType(MSG_I_ANALOG))
					{
						fast << ControllersRaw[0].LeftStickX << ControllersRaw[0].LeftStickY;
						sendInput.LeftStickX = ControllersRaw[0].LeftStickX;
						sendInput.LeftStickY = ControllersRaw[0].LeftStickY;
					}
				}

				/*analogTimer = millisecs();
			}*/
			}
			else if (sendInput.LeftStickY != 0 || sendInput.LeftStickX != 0)
			{
				if (safe.addType(MSG_I_ANALOG))
				{
					cout << "<< Resetting analog" << endl;
					sendInput.LeftStickY = 0;
					sendInput.LeftStickX = 0;
					safe << sendInput.LeftStickY << sendInput.LeftStickX;
				}
			}
		}
	}

	Globals::Networking.Send(fast);
	Globals::Networking.Send(safe);
}
void MemoryHandler::SendPlayer()
{
	PacketEx safe(true), fast(false);

	// If the game has finished loading...
	if (GameState >= GameState::LOAD_FINISHED && TwoPlayerMode > 0)
	{
		// Check if the stage has changed so we can re->valuate the player.
		if (local.game.CurrentLevel != CurrentLevel)
		{
			cout << "<> Stage has changed to " << (ushort)CurrentLevel << endl;

			updateAbstractPlayer(&sendPlayer, Player1);

			// Reset the ringcounts so they don't get sent.
			local.game.RingCount[0] = 0;
			local.game.RingCount[1] = 0;

			// Reset specials
			for (int i = 0; i < 3; i++)
				local.game.P2SpecialAttacks[i] = 0;

			// And finally, update the stage so this doesn't loop.
			local.game.CurrentLevel = CurrentLevel;
		}

		if (CheckTeleport())
		{
			// Send a teleport message
			if (!fast.isInPacket(MSG_P_POSITION) && safe.addType(MSG_P_POSITION))
			{
				safe << Player1->Data1->Position;
				sendPlayer.Data1.Position = Player1->Data1->Position;
			}
			if (!fast.isInPacket(MSG_P_SPEED) && safe.addType(MSG_P_SPEED))
			{
				safe << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
				sendPlayer.Data2.HSpeed = Player1->Data2->HSpeed;
				sendPlayer.Data2.VSpeed = Player1->Data2->VSpeed;
				sendPlayer.Data2.PhysData.BaseSpeed = Player1->Data2->PhysData.BaseSpeed;
			}
		}


		if (memcmp(&local.game.P1SpecialAttacks[0], &P1SpecialAttacks[0], sizeof(char) * 3) != 0)
		{
			if (safe.addType(MSG_S_2PSPECIALS))
			{
				cout << "<< Sending specials!" << endl;
				for (uchar i = 0; i < 3; i++)
					safe << P1SpecialAttacks[i];
				memcpy(&local.game.P1SpecialAttacks[0], &P1SpecialAttacks[0], sizeof(char) * 3);
			}
		}
		if (Player1->Data2->CharID == 6 || Player1->Data2->CharID == 7)
		{
			if (sendPlayer.Data2.MechHP != Player1->Data2->MechHP)
			{
				if (safe.addType(MSG_P_HP))
				{
					safe << Player1->Data2->MechHP;
				}
			}
		}

		if (sendPlayer.Data1.Action != Player1->Data1->Action || sendPlayer.Data1.Status != Player1->Data1->Status)
		{
			cout << "<< Sending action..." << endl;

			bool sendSpinTimer = (Player1->Data2->CharID2 == Characters_Sonic
				|| Player1->Data2->CharID2 == Characters_Sonic
				|| Player1->Data2->CharID2 == Characters_Amy
				|| Player1->Data2->CharID2 == Characters_MetalSonic);

			if (!fast.isInPacket(MSG_P_POSITION) && safe.addType(MSG_P_POSITION))
				safe << Player1->Data1->Position;
			if (safe.addType(MSG_P_ACTION))
				safe << Player1->Data1->Action;
			if (safe.addType(MSG_P_STATUS))
				safe << Player1->Data1->Status;
			if (safe.addType(MSG_P_ANIMATION))
				safe << Player1->Data2->AnimInfo.Next;
			if (sendSpinTimer && safe.addType(MSG_P_SPINTIMER))
				safe << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
		}

		if (local.game.RingCount[0] != RingCount[0])
		{
			if (fast.addType(MSG_P_RINGS))
			{
				cout << "<< Sending rings (" << local.game.RingCount[0] << ")" << endl;
				local.game.RingCount[0] = RingCount[0];
				safe << local.game.RingCount[0];
			}
		}
		/*
		if (memcmp(&sendPlayer.Data1.Rotation, &Player1->Data1->Rotation, sizeof(Rotation)) != 0 ||
		(Player1->Data2->HSpeed != sendPlayer.Data2.HSpeed || Player1->Data2->VSpeed != sendPlayer.Data2.VSpeed))
		*/
		if (RotationMargin(sendPlayer.Data1.Rotation, Player1->Data1->Rotation)
			|| (SpeedMargin(sendPlayer.Data2.HSpeed, Player1->Data2->HSpeed) || SpeedMargin(sendPlayer.Data2.VSpeed, Player1->Data2->VSpeed)))
		{
			rotateTimer = speedTimer = millisecs();
			if (!safe.isInPacket(MSG_P_ROTATION) && fast.addType(MSG_P_ROTATION))
				safe << Player1->Data1->Rotation;
			if (!safe.isInPacket(MSG_P_POSITION) && fast.addType(MSG_P_POSITION))
				safe << Player1->Data1->Position;
			if (!safe.isInPacket(MSG_P_SPEED) && fast.addType(MSG_P_SPEED))
				safe << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
		}

		if (sendPlayer.Data2.Powerups != Player1->Data2->Powerups)
		{
			if (safe.addType(MSG_P_POWERUPS))
			{
				cout << "<< Sending powerups" << endl;
				safe << Player1->Data2->Powerups;
				sendPlayer.Data2.Powerups;
			}
		}
		if (sendPlayer.Data2.Upgrades != Player1->Data2->Upgrades)
		{
			if (safe.addType(MSG_P_UPGRADES))
			{
				cout << "<< Sending upgrades" << endl;
				safe << Player1->Data2->Upgrades;
				sendPlayer.Data2.Upgrades = Player1->Data2->Upgrades;
			}
		}

		updateAbstractPlayer(&sendPlayer, Player1);
	}

	Globals::Networking.Send(fast);
	Globals::Networking.Send(safe);
}
void MemoryHandler::SendMenu()
{
	if (GameState == GameState::INACTIVE)
	{
		PacketEx safe(true), fast(false);

		// Menu analog failsafe
		if (sendInput.LeftStickX != 0 || sendInput.LeftStickY != 0)
		{
			cout << "<>\tAnalog failsafe!" << endl;
			ControllersRaw[0].LeftStickX = 0;
			ControllersRaw[0].LeftStickY = 0;
			sendInput.LeftStickX = 0;
			sendInput.LeftStickY = 0;
		}

		// ...and we're on the 2P menu...
		if (CurrentMenu[0] == Menu::BATTLE)
		{
			firstMenuEntry = (local.menu.sub != CurrentMenu[1]);

			if (memcmp(local.menu.BattleOptions, BattleOptions, sizeof(char) * 4) != 0)
			{
				if (safe.addType(MSG_S_BATTLEOPT))
				{
					cout << "<< Sending battle options..." << endl;
					memcpy(&local.menu.BattleOptions, BattleOptions, sizeof(char) * 4);
					safe.append(&local.menu.BattleOptions[0], sizeof(char) * 4);
				}

				if (safe.addType(MSG_M_BATTLEOPTSEL))
				{
					local.menu.BattleOptionsSelection = BattleOptionsSelection;
					local.menu.BattleOptionsBackSelected = BattleOptionsBackSelected;
					safe << local.menu.BattleOptionsSelection << local.menu.BattleOptionsBackSelected;
				}
			}

			else if (CurrentMenu[1] == SubMenu2P::S_BATTLEOPT)
			{
				if (local.menu.BattleOptionsSelection != BattleOptionsSelection || local.menu.BattleOptionsBackSelected != BattleOptionsBackSelected || firstMenuEntry && Globals::Networking.isServer())
				{
					if (safe.addType(MSG_M_BATTLEOPTSEL))
					{
						local.menu.BattleOptionsSelection = BattleOptionsSelection;
						local.menu.BattleOptionsBackSelected = BattleOptionsBackSelected;
						safe << local.menu.BattleOptionsSelection << local.menu.BattleOptionsBackSelected;
					}
				}
			}


			// ...and we haven't pressed start
			else if (cAt2PMenu[0] = (CurrentMenu[1] == SubMenu2P::S_START && P2Start == 0))
			{
				if (cAt2PMenu[0] && cAt2PMenu[1] && !wroteP2Start)
				{
					P2Start = 2;
					wroteP2Start = true;
				}
			}
			// ...and we HAVE pressed start
			else if (CurrentMenu[1] == SubMenu2P::S_READY || CurrentMenu[1] == SubMenu2P::O_READY)
			{
				if (local.menu.PlayerReady[0] != PlayerReady[0])
				{
					if (safe.addType(MSG_S_2PREADY))
					{
						safe << PlayerReady[0];
						local.menu.PlayerReady[0] = PlayerReady[0];
					}
				}
			}
			else if (CurrentMenu[1] == SubMenu2P::S_BATTLEMODE || firstMenuEntry && Globals::Networking.isServer())
			{
				if (local.menu.BattleSelection != BattleSelection)
				{
					if (safe.addType(MSG_M_BATTLEMODESEL))
					{
						local.menu.BattleSelection = BattleSelection;
						safe << BattleSelection;
					}
				}
			}
			// Character Selection
			else if (CurrentMenu[1] == SubMenu2P::S_CHARSEL || CurrentMenu[1] == SubMenu2P::O_CHARSEL)
			{
				if (CharacterSelected[0] && CharacterSelected[1] && CurrentMenu[1] == SubMenu2P::S_CHARSEL)
				{
					cout << "<> Resetting character selections" << endl;
					CharacterSelectTimer = 0;
					CurrentMenu[1] = SubMenu2P::O_CHARSEL;
				}


				if (local.menu.CharacterSelection[0] != CharacterSelection[0] || firstMenuEntry)
				{
					cout << "<< Sending character selection" << endl;
					if (safe.addType(MSG_M_CHARSEL))
					{
						safe << CharacterSelection[0];
						local.menu.CharacterSelection[0] = CharacterSelection[0];
					}
				}
				if (local.menu.CharacterSelected[0] != CharacterSelected[0])
				{
					if (safe.addType(MSG_M_CHARCHOSEN))
					{
						safe << CharacterSelected[0];
						local.menu.CharacterSelected[0] = CharacterSelected[0];
					}
				}
				if (firstMenuEntry ||
					((local.menu.AltCharacterSonic != AltCharacterSonic) ||
					(local.menu.AltCharacterShadow != AltCharacterShadow) ||
					(local.menu.AltCharacterTails != AltCharacterTails) ||
					(local.menu.AltCharacterEggman != AltCharacterEggman) ||
					(local.menu.AltCharacterKnuckles != AltCharacterKnuckles) ||
					(local.menu.AltCharacterRouge != AltCharacterRouge))
					)
				{
					if (safe.addType(MSG_M_ALTCHAR))
					{
						local.menu.AltCharacterSonic = AltCharacterSonic;
						local.menu.AltCharacterShadow = AltCharacterShadow;
						local.menu.AltCharacterTails = AltCharacterTails;
						local.menu.AltCharacterEggman = AltCharacterEggman;
						local.menu.AltCharacterKnuckles = AltCharacterKnuckles;
						local.menu.AltCharacterRouge = AltCharacterRouge;

						safe.append(&local.menu.AltCharacterSonic, sizeof(char) * 6);
					}
				}
			}
			else if (CurrentMenu[1] == SubMenu2P::I_STAGESEL || CurrentMenu[1] == SubMenu2P::S_STAGESEL)
			{
				if ((memcmp(&local.menu.StageSelection2P[0], &StageSelection2P[0], (sizeof(int) * 2)) != 0 || local.menu.BattleOptionsButton != BattleOptionsButton)
					|| firstMenuEntry)
				{
					if (safe.addType(MSG_M_STAGESEL))
					{
						safe << StageSelection2P[0] << StageSelection2P[1] << BattleOptionsButton;
						local.menu.StageSelection2P[0] = StageSelection2P[0];
						local.menu.StageSelection2P[1] = StageSelection2P[1];
						local.menu.BattleOptionsButton = BattleOptionsButton;
					}
				}
			}
		}
		else
		{
			wroteP2Start = false;
			if (cAt2PMenu[0])
				cAt2PMenu[0] = false;
		}

		if (cAt2PMenu[0] != lAt2PMenu[0])
		{
			if (safe.addType(MSG_M_ATMENU))
			{
				cout << "<< Sending \"On 2P Menu\" state" << endl;
				safe << cAt2PMenu[0];
				lAt2PMenu[0] = cAt2PMenu[0];
			}
		}
		local.menu.sub = CurrentMenu[1];

		Globals::Networking.Send(fast);
		Globals::Networking.Send(safe);
	}
}

inline void MemoryHandler::writeP2Memory()
{
	if (GameState >= GameState::INGAME)
		PlayerObject::WritePlayer(Player2, &recvPlayer);
}

bool MemoryHandler::ReceiveInput(uchar type, sf::Packet& packet)
{
	if (CurrentMenu[0] == 16 || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

		case MSG_I_BUTTONS:
			packet >> recvInput.HeldButtons;
			if (CheckFrame())
				MemManage::waitFrame(1, thisFrame);
			recvInput.WriteButtons(ControllersRaw[1]);

			return true;

		case MSG_I_ANALOG:
			packet >> ControllersRaw[1].LeftStickX >> ControllersRaw[1].LeftStickY;
			return true;
		}
	}

	return false;
}
bool MemoryHandler::ReceiveSystem(uchar type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		switch (type)
		{
		default:
			return false;

		case MSG_S_TIME:
			uchar minute, second, frame;
			packet >> minute >> second >> frame;
			TimerMinutes = local.game.TimerMinutes = minute;
			TimerSeconds = local.game.TimerSeconds = second;
			TimerFrames = local.game.TimerFrames = frame;
			return true;

		case MSG_S_GAMESTATE:
		{
			uchar recvGameState;
			packet >> recvGameState;

			if (GameState >= GameState::INGAME && recvGameState > GameState::LOAD_FINISHED)
				GameState = local.system.GameState = recvGameState;

			return true;
		}

		case MSG_S_PAUSESEL:
			packet >> local.system.PauseSelection;
			PauseSelection = local.system.PauseSelection;
			return true;

		case MSG_S_TIMESTOP:
		{
			uchar temp;
			packet >> temp;
			local.game.TimeStopMode = temp;
			writeTimeStop();
			return true;
		}

		case MSG_S_2PSPECIALS:
		{
			uchar specials[3];
			for (int i = 0; i < 3; i++)
				packet >> specials[i];

			memcpy(local.game.P2SpecialAttacks, specials, sizeof(char) * 3);

			writeSpecials();
			return true;
		}

		case MSG_P_RINGS:
			packet >> local.game.RingCount[1];
			writeRings();

			cout << ">> Ring Count Change: " << local.game.RingCount[1] << endl;
			return true;
		}
	}
	return false;
}
bool MemoryHandler::ReceivePlayer(uchar type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		writePlayer = false;
		switch (type)
		{
		default:
			return false;

		case MSG_P_HP:
			packet >> recvPlayer.Data2.MechHP;
			cout << ">> Received HP update. (" << recvPlayer.Data2.MechHP << ')' << endl;
			writePlayer = true;
			break;

		case MSG_P_ACTION:
		{
			uchar action;
			packet >> action;
			recvPlayer.Data1.Action = action;
			writePlayer = true;
			break;
		}

		case MSG_P_STATUS:
			packet >> recvPlayer.Data1.Status;
			writePlayer = true;
			break;

		case MSG_P_SPINTIMER:
			packet >> recvPlayer.Sonic.SpindashTimer;
			writePlayer = true;
			break;

		case MSG_P_ANIMATION:
			packet >> recvPlayer.Data2.AnimInfo.Next;
			writePlayer = true;
			break;

		case MSG_P_POSITION:
			packet >> recvPlayer.Data1.Position;
			writePlayer = true;
			break;

		case MSG_P_ROTATION:
			packet >> recvPlayer.Data1.Rotation;
			writePlayer = true;
			break;

		case MSG_P_SPEED:
			packet >> recvPlayer.Data2.HSpeed;
			packet >> recvPlayer.Data2.VSpeed;
			packet >> recvPlayer.Data2.PhysData.BaseSpeed;

			writePlayer = true;
			break;

		case MSG_P_POWERUPS:
		{
			short powerups;
			packet >> powerups;
			recvPlayer.Data2.Powerups = (Powerups)powerups;
			writePlayer = true;
			break;
		}

		case MSG_P_UPGRADES:
		{
			int upgrades;
			packet >> upgrades;
			recvPlayer.Data2.Upgrades = (Upgrades)upgrades;
			writePlayer = true;
			break;
		}
		}

		if (writePlayer)
			writeP2Memory();

		return writePlayer;
	}

	return false;
}
bool MemoryHandler::ReceiveMenu(uchar type, sf::Packet& packet)
{
	if (GameState == GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

		case MSG_M_ATMENU:
			packet >> cAt2PMenu[1];
			if (cAt2PMenu[1])
				cout << ">> Player 2 is ready on the 2P menu!" << endl;
			else
				cout << ">> Player 2 is no longer on the 2P menu." << endl;
			return true;

		case MSG_S_2PREADY:
			packet >> local.menu.PlayerReady[1];
			PlayerReady[1] = local.menu.PlayerReady[1];

			cout << ">> Player 2 ready state changed." << endl;
			return true;

		case MSG_M_CHARSEL:
			packet >> local.menu.CharacterSelection[1];
			CharacterSelection[1] = local.menu.CharacterSelection[1];

			return true;

		case MSG_M_CHARCHOSEN:
			packet >> local.menu.CharacterSelected[1];
			CharacterSelected[1] = local.menu.CharacterSelected[1];
			return true;

		case MSG_M_ALTCHAR:
			packet >> local.menu.AltCharacterSonic
				>> local.menu.AltCharacterShadow
				>> local.menu.AltCharacterTails
				>> local.menu.AltCharacterEggman
				>> local.menu.AltCharacterKnuckles
				>> local.menu.AltCharacterRouge;
			AltCharacterSonic = local.menu.AltCharacterSonic;
			AltCharacterShadow = local.menu.AltCharacterShadow;

			AltCharacterTails = local.menu.AltCharacterTails;
			AltCharacterEggman = local.menu.AltCharacterEggman;

			AltCharacterKnuckles = local.menu.AltCharacterKnuckles;
			AltCharacterRouge = local.menu.AltCharacterRouge;

			return true;

		case MSG_S_BATTLEOPT:
			for (int i = 0; i < 4; i++)
				packet >> local.menu.BattleOptions[i];
			memcpy(BattleOptions, local.menu.BattleOptions, sizeof(char) * 4);

			return true;

		case MSG_M_BATTLEOPTSEL:
			packet >> local.menu.BattleOptionsSelection
				>> local.menu.BattleOptionsBackSelected;
			BattleOptionsSelection = local.menu.BattleOptionsSelection;
			BattleOptionsBackSelected = local.menu.BattleOptionsBackSelected;

			return true;

		case MSG_M_STAGESEL:
			packet >> local.menu.StageSelection2P[0]
				>> local.menu.StageSelection2P[1]
				>> local.menu.BattleOptionsButton;
			StageSelection2P[0] = local.menu.StageSelection2P[0];
			StageSelection2P[1] = local.menu.StageSelection2P[1];
			BattleOptionsButton = local.menu.BattleOptionsButton;

			return true;

		case MSG_M_BATTLEMODESEL:
			packet >> local.menu.BattleSelection;
			BattleSelection = local.menu.BattleSelection;

			return true;
		}
	}
	return false;
}

void MemoryHandler::PreReceive()
{
	updateAbstractPlayer(&recvPlayer, Player2);

	writeRings();
	writeSpecials();

	return;
}
void MemoryHandler::PostReceive()
{
	updateAbstractPlayer(&recvPlayer, Player2);
	//writeP2Memory();

	writeRings();
	writeSpecials();

	return;
}

inline void MemoryHandler::writeRings() { RingCount[1] = local.game.RingCount[1]; }
inline void MemoryHandler::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3); }
inline void MemoryHandler::writeTimeStop() { TimeStopMode = local.game.TimeStopMode; }

void MemoryHandler::updateAbstractPlayer(PlayerObject* destination, ObjectMaster* source)
{
	// Mech synchronize hack
	if (GameState >= GameState::INGAME && Player2 != nullptr)
	{
		if (Player2->Data2->CharID2 == Characters_MechEggman || Player2->Data2->CharID2 == Characters_MechTails)
			Player2->Data2->MechHP = recvPlayer.Data2.MechHP;
		Player2->Data2->Powerups = recvPlayer.Data2.Powerups;
		Player2->Data2->Upgrades = recvPlayer.Data2.Upgrades;
	}

	destination->Set(source);
}

void MemoryHandler::ToggleSplitscreen()
{
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((sendInput.HeldButtons & (1 << 16)) && (sendInput.HeldButtons & (2 << 16)))
		{
			if (!splitToggled)
			{
				if (SplitscreenMode == 1)
					SplitscreenMode = 2;
				else if (SplitscreenMode == 2)
					SplitscreenMode = 1;
				else
					return;
				splitToggled = true;
			}
		}

		else if (splitToggled)
			splitToggled = false;
	}

	return;
}
bool MemoryHandler::CheckTeleport()
{
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((sendInput.HeldButtons & (1 << 5)) && (sendInput.HeldButtons & (1 << 9)))
		{
			if (!Teleported)
			{
				// Teleport to recvPlayer
				cout << "<> Teleporting to other player..." << endl;;
				PlayerObject::Teleport(&recvPlayer, Player1);

				return Teleported = true;
			}
		}
		else if (Teleported)
			return Teleported = false;
	}

	return false;
}