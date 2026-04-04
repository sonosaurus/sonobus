# Makefile for doku files and 

DOCS=aoo_protocol.pdf aoo_usecases.pdf faq.pdf

all: ${DOCS}

%.tex : %.rst
	rst2latex $< > $@

%.pdf: %.tex
	pdflatex $<
	pdflatex $<

clean:
	rm -f *.out *aux *bbl *blg *log *toc *.ptb *.tod *.fls *.fdb_latexmk *.lof

distclean: clean
	rm -f ${DOCS} 
