#include "stdafx.h"
#include "chao.h"
#include "ChaoMaybe.h"
#include "globals.h"

using namespace nethax;

static Trampoline* SpawnAllChaoInGarden_t = nullptr;

DataArray(ALFSave, ChaoSave, 0x019F6460, 2);
static constexpr auto chao_slot_count = 24;

static bool received_chao[chao_slot_count] {};


bool all_initialized()
{
	return std::all_of(std::begin(received_chao), std::end(received_chao), [](auto a) -> auto
	{
		return a;
	});
}

void mark_chao()
{
	for (auto& value : received_chao)
	{
		value = false;
	}
}

static void __cdecl SpawnAllChaoInGarden_r()
{
	if (!nethax::globals::is_connected())
	{
		return;
	}

	if (globals::networking->is_server())
	{
		ushort i = 0;
		std::vector<uint8_t> buffer(sizeof(ChaoData_));

		for (const auto& slot : ChaoSave[0].ChaoSlots)
		{
			sws::Packet packet;
			memcpy(buffer.data(), &slot, sizeof(ChaoData_));
			packet << i++ << buffer;
			globals::broker->append(MessageID::C_ChaoData, Protocol::tcp, &packet, true);
		}

		globals::broker->finalize();

		//if (!globals::broker->wait_for_players(MessageID::C_ChaoSpawned))
		//{
		//	throw; // SHIT SHIT ABORT
		//}

		PrintDebug("shfjkasdhflkjsahfkjashdf");
	}
	else
	{
		while (!all_initialized())
		{
			globals::broker->receive_loop();
		}

		mark_chao();
		//globals::broker->send_ready_and_wait(MessageID::C_ChaoSpawned);
	}

	auto original = reinterpret_cast<decltype(SpawnAllChaoInGarden_r)*>(SpawnAllChaoInGarden_t->Target());
	original();
}

bool message_handler(nethax::MessageID id, pnum_t pnum, sws::Packet& packet)
{
	if (id != MessageID::C_ChaoData)
	{
		return false;
	}

	ushort chao_id = 0;
	std::vector<uint8_t> buffer;

	packet >> chao_id;
	packet >> buffer;

	if (chao_id >= chao_slot_count)
	{
		return false;
	}

	received_chao[chao_id] = true;
	memcpy(&ChaoSave[0].ChaoSlots[chao_id], buffer.data(), buffer.size());

	return true;
}

namespace events
{
	void InitChao()
	{
		SpawnAllChaoInGarden_t = new Trampoline(0x00531B10, 0x00531B18, SpawnAllChaoInGarden_r);
		globals::broker->register_message_handler(MessageID::C_ChaoData, &message_handler);
	}
}
