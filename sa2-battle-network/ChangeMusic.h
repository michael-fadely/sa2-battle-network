#pragma once
#include <SA2Functions.h>

// Immediately changes the music to the specified song.
// Automatically calls StopMusic() and ResetMusic().
inline void ChangeMusic(const char* song)
{
	StopMusic();
	PlayMusic(song);
	ResetMusic();
}
