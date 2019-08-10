#pragma once

#define CHECK_SIZE(NAME) static_assert(sizeof(NAME) == sizeof(:: ## NAME), "size mismatch: " # NAME)
