// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

namespace SysInfo {

const char* getPlatformID();
const char* getSystemID();
const char* getCompilerID();
int getBitness();

}  // namespace SysInfo
