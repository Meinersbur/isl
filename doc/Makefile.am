if GENERATE_DOC
export TEXINPUTS := $(srcdir):$(TEXINPUTS)
export BIBINPUTS := $(srcdir):$(BIBINPUTS)
export BSTINPUTS := $(srcdir):$(BSTINPUTS)

user.tex: user.pod
	$(PERL) $(srcdir)/mypod2latex $< $@
manual.pdf: manual.tex user.tex $(srcdir)/implementation.tex
	$(PDFLATEX) $<
	bibtex manual
	$(PDFLATEX) $<
	$(PDFLATEX) $<
user.html: user.pod
	$(POD2HTML) --infile=$< --outfile=$@ --title="Integer Set Library: Manual"
endif
