# set default file name
if (NOT "${GIT_REV_FILE}")
    set (GIT_REV_FILE "${CMAKE_CURRENT_LIST_DIR}/src/git_rev.c")
endif ()

# get full commit hash
execute_process (
    COMMAND git rev-parse HEAD
    OUTPUT_VARIABLE GIT_REV_FULL
    ERROR_QUIET
)

if (GIT_REV_FULL)
    # abbreviate hash to 7 nibbles
    string (SUBSTRING "${GIT_REV_FULL}" 0 7 GIT_REV)

    # append '+' if there are modifications
    execute_process (
        COMMAND git diff --quiet --exit-code
        RESULT_VARIABLE IS_MODIFIED
    )
    if (IS_MODIFIED)
        set (GIT_REV "${GIT_REV}+")
    endif ()

    # report version on the console
    message (STATUS "Git revision: ${GIT_REV}")

    # get the branch (if there's any)
    execute_process (
        COMMAND git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH
    )
    if (NOT "${GIT_BRANCH}" STREQUAL "${GIT_REV_FULL}")
        string (STRIP "${GIT_BRANCH}" GIT_BRANCH)
        set (GIT_BRANCH "\"${GIT_BRANCH}\"")  # convert to C string
    else ()
        set (GIT_BRANCH "0")
    endif ()

    # set the final file contents
    set (FILE_CONTENTS "// this file is auto-generated, don't edit it
const char* git_rev = \"${GIT_REV}\";
const char* git_branch = ${GIT_BRANCH};
")

else ()
    # fallback if the initial Git query failed
    message (STATUS "not a Git checkout")
    set (FILE_CONTENTS "// not a Git checkout
const char* git_rev = nullptr;
const char* git_branch = nullptr;
")
endif ()

# read original file contents
if (EXISTS "${GIT_REV_FILE}")
    file (READ "${GIT_REV_FILE}" OLD_CONTENTS)
else ()
    set (OLD_CONTENTS "xxx")
endif ()

# replace file with new contents
if (NOT "${FILE_CONTENTS}" STREQUAL "{OLD_CONTENTS}")
    file (WRITE "${GIT_REV_FILE}" "${FILE_CONTENTS}")
endif ()
