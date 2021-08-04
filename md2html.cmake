# sanity check: required parameters present?
if (NOT FILE)
    message (FATAL_ERROR "usage: cmake -DFILE=file_without_suffixes [-DTITLE=\"Page Title\"] [-DREMOVE=Section] [-DPANDOC=/path/to/pandoc] -P md2html.cmake")
endif ()

# set file names
set (INFILE  "${FILE}.md")
set (TMPFILE "${FILE}.tmp.md")
set (OUTFILE "${FILE}.html")

# sanity check: valid input file name?
if (NOT EXISTS "${INFILE}")
    message (FATAL_ERROR "input file '${INFILE}' doesn't exist")
endif ()

# use local or system-wide pandoc installation if not specified otherwise
if (NOT PANDOC)
    if (WIN32)
        set (PANDOC "pandoc.exe")
    else ()
        set (PANDOC "pandoc")
    endif ()
    # search for a local installation, and if one exists, use it
    file (GLOB PANDOCS
          LIST_DIRECTORIES false
          "${CMAKE_CURRENT_LIST_DIR}/pandoc-*/${PANDOC}")
    if (PANDOCS)
        set (PANDOC "${PANDOCS}")
    endif ()
endif ()

# modify file
file (READ "${INFILE}" DATA)
if (REMOVE)
    string (REGEX REPLACE "\n## ${REMOVE}.*\n## " "\n## " DATA "${DATA}")
endif ()
string (REGEX REPLACE "\\.md\\)" ".html)" DATA "${DATA}")
file (WRITE "${TMPFILE}" "${DATA}")
set (INFILE "${TMPFILE}")

# run pandoc
execute_process (
    COMMAND "${PANDOC}" -f markdown -t html5 -s --self-contained -T "${TITLE}" -V "mainfont=\"Fira Sans\",\"Segoe UI\",Roboto,\"Noto Sans\",sans-serif" -V fontsize=16px -V sourcefile= "${INFILE}" -o "${OUTFILE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    RESULT_VARIABLE RES
    ERROR_VARIABLE ERRORS
)
file (REMOVE "${TMPFILE}")
if (NOT RES EQUAL 0)
    if ("${ERRORS}" STREQUAL "")
        message (FATAL_ERROR "running pandoc failed with exit code '${RES}'")
    else ()
        message (FATAL_ERROR "running pandoc failed with exit code '${RES}' and message:\n${ERRORS}")
    endif ()
endif ()
