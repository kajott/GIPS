#pragma once

#include <string>
#include <vector>

std::vector<std::string> pfd_open_file_wrapper(
    std::string const &title,
    std::string const &default_path,
    std::vector<std::string> const &filters);

std::string pfd_save_file_wrapper(
    std::string const &title,
    std::string const &default_path,
    std::vector<std::string> const &filters);
