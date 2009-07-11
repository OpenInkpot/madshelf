INCLUDES = @EFL_CFLAGS@ @EXTRACT_CFLAGS@ @SQLITE_CFLAGS@ @ECHOICEBOX_CFLAGS@ @EOI_CFLAGS@
AM_CFLAGS=-Wall -Wno-unused -Werror -DSYSCONFDIR="\"$(sysconfdir)\""

bin_PROGRAMS = madshelf

madshelf_SOURCES = main.c overview.c tags.c disks.c utils.c favorites.c recent.c fileinfo.c dir.c madshelf_extractors.c fileinfo_render.c file_context_menu.c battery.c handlers.c screen_context_menu.c run.c filters.c clock.c
madshelf_LDADD = @EFL_LIBS@ @EXTRACT_LIBS@ @SQLITE_LIBS@ @ECHOICEBOX_LIBS@ @EOI_LIBS@

