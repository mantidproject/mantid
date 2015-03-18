all: Versions.html guide.html guide.pdf

IMAGES=x11.png qt.png qt2.png win32.png
PRODUCTS=index.html Versions.html guide.html guide.pdf $(IMAGES) ../Versions
DEST=/home/groups/c/cx/cxxtest/htdocs/

upload: all
	scp $(PRODUCTS) erez_v@cxxtest.sourceforge.net:$(DEST)

copy: all
	cp $(PRODUCTS) $(DEST)

clean:
	rm -f Versions.html guide.html *.css *.aux *.dvi *.log *.toc *.old *~ *.pdf *.ps *.eps *.info *.tmp WARNINGS

Versions.html Versions.texi: ../Versions convert.pl
	perl -w convert.pl $< Versions.html Versions.texi

guide.html: $(IMAGES)

guide.pdf: $(IMAGES)

%.html: %.texi Versions.texi
	makeinfo --html --no-split --number-sections --output=$@ $<

%.pdf: %.texi Versions.texi texinfo.tex
	texi2pdf $<

