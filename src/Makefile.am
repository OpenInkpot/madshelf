INCLUDES = @EFL_CFLAGS@ @EXTRACT_CFLAGS@ @SQLITE_CFLAGS@ @LIBCHOICEBOX_CFLAGS@ @LIBEOI_CFLAGS@ @LIBKEYS_CFLAGS@
AM_CFLAGS=-std=gnu99 -Wall -Wno-unused -Werror -DSYSCONFDIR="\"$(sysconfdir)\"" -DTHEMEDIR='"$(datadir)/madshelf"'

bin_PROGRAMS = madshelf madaudio

madaudio_SOURCES = madaudio.c madaudio_cmd.c madaudio_play.c madaudio_draw.c battery.c clock.c empd_io.c empd_callback.c empd_playlist.c empd_recv.c empd_send.c
madaudio_LDADD = @EFL_LIBS@ @EXTRACT_LIBS@ @SQLITE_LIBS@ @LIBCHOICEBOX_LIBS@ @LIBEOI_LIBS@ @EXTRACT_LIBS@ @LIBKEYS_LIBS@ -lmpdclient


madshelf_SOURCES = main.c overview.c tags.c disks.c utils.c favorites.c recent.c fileinfo.c dir.c fileinfo_render.c file_context_menu.c battery.c handlers.c screen_context_menu.c run.c filters.c clock.c delete_file.c curdir.c empd_io.c empd_callback.c empd_playlist.c empd_recv.c audio.c empd_send.c
madshelf_LDADD = @EFL_LIBS@ @EXTRACT_LIBS@ @SQLITE_LIBS@ @LIBCHOICEBOX_LIBS@ @LIBEOI_LIBS@ @EXTRACT_LIBS@ @LIBKEYS_LIBS@ -lmpdclient

