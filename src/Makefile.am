INCLUDES = @EFL_CFLAGS@ @EXTRACT_CFLAGS@ @SQLITE_CFLAGS@ @LIBCHOICEBOX_CFLAGS@ @LIBEOI_CFLAGS@ @LIBKEYS_CFLAGS@
AM_CFLAGS=-std=gnu99 -Wall -Wno-unused -Werror -DSYSCONFDIR="\"$(sysconfdir)\"" -DTHEMEDIR='"$(datadir)/madshelf"'

bin_PROGRAMS = madshelf test_mime

madshelf_SOURCES = main.c overview.c tags.c disks.c utils.c favorites.c recent.c fileinfo.c dir.c fileinfo_render.c file_context_menu.c battery.c handlers.c screen_context_menu.c run.c filters.c clock.c delete_file.c curdir.c text_escape.c app_defaults.c
madshelf_LDADD = @EFL_LIBS@ @EXTRACT_LIBS@ @SQLITE_LIBS@ @LIBCHOICEBOX_LIBS@ @LIBEOI_LIBS@ @EXTRACT_LIBS@ @LIBKEYS_LIBS@
madshelf_LDFLAGS = -Wl,--as-needed

test_mime_SOURCES = test_mime.c app_defaults.c
test_mime_LDADD = @EFL_LIBS@
