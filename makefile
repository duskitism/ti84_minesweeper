NAME = MINESW
ICON = bomb_icon.png
DESCRIPTION = "boom!"

COMPRESSED = YES
COMPRESSED_MODE = zx0

ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

include $(shell cedev-config --makefile)
