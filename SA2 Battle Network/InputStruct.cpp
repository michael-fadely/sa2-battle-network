#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Common.h"
#include "LazyMemory.h"
#include "MemoryManagement.h"

#include "InputStruct.h"

using namespace std;

InputStruct::InputStruct(uint baseAddress)
{
	if (!MemManage::InputInitalized())
	{
		cout << "[InputStruct::InputStruct] ";
		MemManage::waitInputInit();
	}

	ReadMemory(baseAddress, &this->baseAddress, sizeof(int));

	buttons.Clear = 0x0000;

	memset(&buttons,	0x00, sizeof(inputButtons));
	memset(&analog,		0x00, sizeof(inputAnalog));

	cout << "[InputStruct::InputStruct] Initialized input struct with address " << hex << this->baseAddress << dec << endl;
	return;
}

void InputStruct::read()
{
	ReadMemory((baseAddress+Offset::Held), &buttons.Held, sizeof(int));
	buttons.NotHeld = ~buttons.Held;

	ReadMemory((baseAddress+Offset::AnalogX), &analog.x, sizeof(short));
	ReadMemory((baseAddress+Offset::AnalogY), &analog.y, sizeof(short));
}

void InputStruct::write(abstractInput* recvr)
{
	writeButtons(recvr);
	writeAnalog(recvr, 1);
}

void InputStruct::writeButtons(abstractInput* recvr)
{
	buttons.Held = recvr->buttons.Held;
	buttons.NotHeld = ~buttons.Held;

	WriteMemory((baseAddress+Offset::Held),		&buttons.Held,		sizeof(int));
	WriteMemory((baseAddress+Offset::NotHeld),	&buttons.NotHeld,	sizeof(int));
	press();
	//thread (&InputStruct::press, this).detach();
}


void InputStruct::writeAnalog(abstractInput* recvr, uint gamestate)
{
	analog.x = recvr->analog.x;
	analog.y = recvr->analog.y;
	WriteMemory((baseAddress+Offset::AnalogX), &analog.x, sizeof(short));
	WriteMemory((baseAddress+Offset::AnalogY), &analog.y, sizeof(short));
	if (gamestate == 0)
	{
		ushort none = 0;
		MemManage::waitFrame();
		WriteMemory((baseAddress+Offset::AnalogX), &none, sizeof(short));
		WriteMemory((baseAddress+Offset::AnalogY), &none, sizeof(short));
	}

}


void InputStruct::press()
{
	// Write the currently held buttons, sleep, then clear.
	buttons.Pressed = buttons.Held;
	buttons.Pressed &= ~buttons.LastPressed;
	buttons.LastPressed = buttons.Held;

	WriteMemory((baseAddress+Offset::Pressed), &buttons.Pressed, sizeof(int));
	MemManage::waitFrame();
	WriteMemory((baseAddress+Offset::Pressed), &buttons.Clear, sizeof(int));
	
	/*
	// Write released buttons, sleep, then clear.
	WriteMemory(addrArray[3], &buttons.Pressed, 4);
	waitFrame();
	WriteMemory(addrArray[3], &buttons.Clear, 4);
	*/

	return;
}