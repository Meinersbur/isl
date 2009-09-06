if GENERATE_DOC
user.tex: user.pod
	$(PERL) $(srcdir)/mypod2latex $< $@
manual.pdf: manual.tex user.tex
	$(PDFLATEX) $<
	$(PDFLATEX) $<
user.html: user.pod
	$(POD2HTML) --infile=$< --outfile=$@ --title="Integer Set Library: Manual"
endif
