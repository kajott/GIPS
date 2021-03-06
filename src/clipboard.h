// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Clipboard {

//! initialize the clipboard
void init(GLFWwindow* window);

//! check whether clipboard functionality is supported at all on this platform
bool isAvailable();

//! return the clipboard as a string
//! \returns pointer to the string, malloc()'d internally,
//!          to be free()'d by the caller; or nullptr on failure
char* getString();

//! return the clipboard as a 32-bit (8-bit per channel) RGBA image
//! \returns pointer to the image, malloc()'d internally,
//!          to be free()'d by the caller; or nullptr on failure
//! \note the values returned in width and height are undefined on error
void* getRGBA8Image(int &width, int &height);

//! put a 32-bit (8-bit per channel) RGBA image and (optionally) text onto the clipboard
//! \returns true on success, false on failure
bool setRGBA8ImageAndText(const void* image, int width, int height,
                          const char* text=nullptr, int length=-1);

} // namespace Clipboard
