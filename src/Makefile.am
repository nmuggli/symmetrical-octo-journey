AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libcommon.a libfilter.a
bin_PROGRAMS=rastertobarcodetspl

libcommon_a_SOURCES =	./debug.c			\
						./common.c			\
						./libloader.c		\
						./cupsarray.c		\
						./cupsfile.c		\
						./cupslanguage.c	\
						./devmode.c

libcommon_a_CFLAGS =
libcommon_a_LIBADD =

libfilter_a_SOURCES =	./filter/main.c				\
						./filter/ps2bmp.c				\
						./filter/gsrun.c				\
						./filter/psrun.c				\
						./filter/bmp2tspl.c

libfilter_a_CFLAGS =
libfilter_a_LIBADD = libcommon.a


rastertobarcodetspl_SOURCES  =	./filter/rastertotspl.c	\
						./filter/raster.c			\
						./filter/tspl.c

rastertobarcodetspl_CFLAGS   = -D_TSPL -I.
rastertobarcodetspl_LDFLAGS  = -s
rastertobarcodetspl_LDADD    = libcommon.a

INCLUDES = -I.
