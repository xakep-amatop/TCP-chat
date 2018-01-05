# Search for the path containing library's headers
find_path(Ncurses_ROOT_DIR
	NAMES include/ncurses.h
)

# Search for include directory
find_path(Ncurses_INCLUDE_DIR
	NAMES ncurses.h
	HINTS ${Ncurses_ROOT_DIR}/include
)

# Search for library
find_library(Ncurses_LIBRARY
	NAMES ncurses
	HINTS ${Ncurses_ROOT_DIR}/lib
	)

# Conditionally set READLINE_FOUND value
if(Ncurses_INCLUDE_DIR AND Ncurses_LIBRARY
		AND Ncurses_LIBRARY)
	set(READLINE_FOUND TRUE)
else(Ncurses_INCLUDE_DIR AND Ncurses_LIBRARY
		AND Ncurses_LIBRARY)
	FIND_LIBRARY(Ncurses_LIBRARY NAMES ncurses)
	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG
		Ncurses_INCLUDE_DIR Ncurses_LIBRARY )
	MARK_AS_ADVANCED(Ncurses_INCLUDE_DIR Ncurses_LIBRARY)
endif(Ncurses_INCLUDE_DIR AND Ncurses_LIBRARY
	AND Ncurses_LIBRARY)

# Hide these variables in cmake GUIs
mark_as_advanced(
	Ncurses_ROOT_DIR
	Ncurses_INCLUDE_DIR
	Ncurses_LIBRARY
)
