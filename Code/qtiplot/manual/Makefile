html-pdf:
	docbook2html -d qtiplot.dsl docbook-en/index.docbook -e no-valid
	dblatex -o qtiplot-manual-en.pdf -c dblatex.conf -p dblatex-param.xsl docbook-en/index.docbook
