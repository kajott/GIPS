// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <SDL.h>

namespace Clipboard {

//! initialize the clipboard
void init(SDL_Window* window);

//! check whether clipboard functionality is supported at all on this platform
bool isAvailable();

//! return the clipboard as a 32-bit (8-bit per channel) RGBA image
//! \returns pointer to the image, malloc()'d internally,
//!          to be free()'d by the caller; or nullptr on failure
//! \note the values returned in width and height are undefined on error
void* getRGBA8Image(int &width, int &height);

//! put a 32-bit (8-bit per channel) RGBA image onto the clipboard
//! \returns true on success, false on failure
bool setRGBA8Image(const void* image, int width, int height);

} // namespace Clipboard
