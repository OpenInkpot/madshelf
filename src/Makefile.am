libpositionsdir= "${pkglibdir}/positions"

INCLUDES = @EFL_CFLAGS@ @EXTRACT_CFLAGS@ @SQLITE_CFLAGS@ @LIBCHOICEBOX_CFLAGS@ @LIBEOI_CFLAGS@ @LIBKEYS_CFLAGS@
AM_CFLAGS=-std=gnu99 -Wall -Wno-unused  -DSYSCONFDIR="\"$(sysconfdir)\"" -DDATADIR='"$(datadir)"' -DMADSHELF_PLUGINS_DIR="\"$(libpositionsdir)\""

include_HEADERS = madshelf_positions.h
bin_PROGRAMS = madshelf

madshelf_SOURCES = main.c overview.c tags.c disks.c utils.c favorites.c recent.c fileinfo.c dir.c fileinfo_render.c file_context_menu.c handlers.c screen_context_menu.c run.c filters.c delete_file.c curdir.c text_escape.c app_defaults.c clipboard.c positions.c
madshelf_LDADD = @EFL_LIBS@ @EXTRACT_LIBS@ @SQLITE_LIBS@ @LIBCHOICEBOX_LIBS@ @LIBEOI_LIBS@ @EXTRACT_LIBS@ @LIBKEYS_LIBS@
madshelf_LDFLAGS = -Wl,--as-needed -export-dynamic

libpositions_LTLIBRARIES = positions_fb.la positions_cr3.la

positions_fb_la_SOURCES = positions_fb.c
positions_fb_la_LDFLAGS = -module -avoid-version -shared

positions_cr3_la_SOURCES = positions_cr3.c
positions_cr3_la_LDFLAGS = -module -avoid-version -shared
