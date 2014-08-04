#include "ModLoaderExtensions.h"
#include "MemoryManagement.h"

void InputStruct::WriteButtons(ControllerData& destination)
{
	PressedButtons = HeldButtons;
	PressedButtons &= ~LastPressed;
	LastPressed = HeldButtons;

	destination.HeldButtons = HeldButtons;
	destination.NotHeldButtons = ~destination.HeldButtons;
	destination.PressedButtons = PressedButtons;
	MemManage::waitFrame();
	destination.PressedButtons = 0;
}