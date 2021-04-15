// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "clipboard.h"

void Clipboard::init(GLFWwindow* window) {
    (void)window;
}

bool Clipboard::isAvailable() {
    return false;
}

void* Clipboard::getRGBA8Image(int &width, int &height) {
    width = height = 0;
    return nullptr;
}

bool Clipboard::setRGBA8Image(const void* image, int width, int height) {
    (void)image, (void)width, (void)height;
    return false;
}
