INCLUDES = @EFL_CFLAGS@ @EXTRACT_CFLAGS@ @SQLITE_CFLAGS@ @LIBCHOICEBOX_CFLAGS@ @LIBEOI_CFLAGS@ @LIBKEYS_CFLAGS@
AM_CFLAGS=-std=gnu99 -Wall -Wno-unused  -DSYSCONFDIR="\"$(sysconfdir)\"" -DDATADIR='"$(datadir)"'

bin_PROGRAMS = madshelf

madshelf_SOURCES = main.c overview.c tags.c disks.c utils.c favorites.c recent.c fileinfo.c dir.c fileinfo_render.c file_context_menu.c handlers.c screen_context_menu.c run.c filters.c delete_file.c curdir.c text_escape.c app_defaults.c clipboard.c
madshelf_LDADD = @EFL_LIBS@ @EXTRACT_LIBS@ @SQLITE_LIBS@ @LIBCHOICEBOX_LIBS@ @LIBEOI_LIBS@ @EXTRACT_LIBS@ @LIBKEYS_LIBS@
madshelf_LDFLAGS = -Wl,--as-needed
