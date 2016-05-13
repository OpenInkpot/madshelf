THEMES=$(wildcard $(srcdir)/*.edc)
COMPILED_THEMES=$(patsubst %.edc,%.edj,$(THEMES))

themedir=$(datadir)/eoi/themes/default
theme_DATA=$(COMPILED_THEMES)

%.edj: %.edc
	edje_cc -id $(<D)/.. -id $(<D) $< $@

CLEANFILES=$(COMPILED_THEMES)
