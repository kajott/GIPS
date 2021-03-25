#include "libs.h"

#include "portable-file-dialogs.h"

// The functions in pfd are all marked inline, which causes some optimizers
// to throw them out completely. That's why we declare some wrappers here.

std::vector<std::string> pfd_open_file_wrapper(
    std::string const &title,
    std::string const &default_path,
    std::vector<std::string> const &filters
) {
    return pfd::open_file(title, default_path, filters).result();
}

std::string pfd_save_file_wrapper(
    std::string const &title,
    std::string const &default_path,
    std::vector<std::string> const &filters
) {
    return pfd::save_file(title, default_path, filters).result();
}
