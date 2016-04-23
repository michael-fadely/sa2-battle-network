#pragma once

namespace nethax
{
	namespace events
	{
		void __stdcall SetCurrentLevel(short stage);
		void InitOnStageChange();
		void DeinitOnStageChange();
	}
}
