CMAKE_BUILD_TYPE ?= Release

all: gips

_build:
	mkdir _build

_build/build.ninja:
	cmake -G Ninja -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -S . -B _build

gips: _build/build.ninja src/*
	ninja -C _build

cppcheck:
	cppcheck --std=c99 --std=c++11 --enable=all src

debug:
	$(MAKE) CMAKE_BUILD_TYPE=Debug gips

release:
	$(MAKE) CMAKE_BUILD_TYPE=Release gips

test: gips
	./gips

clean:
	rm -rf _build *.ilk src/git_rev.c

distclean: clean
	rm -f gips gips_debug gips.exe gips_debug.exe *.pdb
	rm -f gips_ui.ini README.html ShaderFormat.html

ultraclean: distclean
	rm -f vswhere.exe ninja.exe
	rm -rf cmake-* pandoc-*

.PHONY: all clean distclean ultraclean cppcheck debug release test
