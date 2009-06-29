INCLUDES = @EXTRACT_CFLAGS@ @SQLITE_CFLAGS@ @EINA_CFLAGS@ @ECORE_EVAS_CFLAGS@ @ECHOICEBOX_CFLAGS@ @EXTRACT_CFLAGS@ @ECORE_CONFIG_CFLAGS@ @EFREET_CFLAGS@ @EFREET_MIME_CFLAGS@ @EDJE_CFLAGS@
AM_CFLAGS=-Wall -Wno-unused -Werror -DSYSCONFDIR="\"$(sysconfdir)\""

bin_PROGRAMS = madshelf

madshelf_SOURCES = main.c overview.c tags.c disks.c utils.c favorites.c recent.c fileinfo.c dir.c madshelf_extractors.c fileinfo_render.c file_context_menu.c battery.c handlers.c screen_context_menu.c run.c filters.c clock.c
madshelf_LDADD = @SQLITE_LIBS@ @EINA_LIBS@ @ECORE_EVAS_LIBS@ @ECHOICEBOX_LIBS@ @EXTRACT_LIBS@ @ECORE_CONFIG_LIBS@ @EFREET_LIBS@ @EFREET_MIME_LIBS@ @EDJE_CFLAGS@
