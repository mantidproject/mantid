from assistant_common import WEB_BASE, HTML_DIR, addEle, addTxtEle
import os
from htmlwriter import HtmlWriter
import mantid.api
from mediawiki import MediaWiki
import wiki_tools
from xml.dom.minidom import Document

def propToList(func, number):
    #htmlfile.writeRow(["Order", "Name", "Default", "Description"], True)
    # order
    result = [str(number+1)]
    # name
    result.append(func.getParamName(number))
    # default
    default = func.getParamValue(number)
    result.append(str(default))
    # description
    text = func.getParamDescr(number)
    if len(text) <= 0:
        text = "&nbsp;"
    else:
        text = text.replace("\n", "<br/>")
    result.append(text)
    return result

def process_function(name, qhp, outputdir, **kwargs): # was (args, algo):
    outfile = "FitFunc_%s.html" % name
    qhp.addFile(os.path.join(HTML_DIR, outfile), name)

    func = mantid.api.FunctionFactory.createFunction(name)
    #print "***", func, dir(func)

    htmlfile = HtmlWriter(os.path.join(outputdir, outfile), name)
#    htmlfile.write("""<!--<script type="text/x-mathjax-config">
#                MathJax.Hub.Config({"HTML-CSS": { preferredFont: "TeX", availableFonts: ["STIX","TeX"], linebreaks: { automatic:true }, EqnChunk: (MathJax.Hub.Browser.isMobile ? 10 : 50) },
#                                    tex2jax: { inlineMath: [ ["$", "$"], ["\\\\(","\\\\)"] ], displayMath: [ ["$$","$$"], ["\\[", "\\]"] ], processEscapes: true, ignoreClass: "tex2jax_ignore|dno" },
#                                    TeX: {  noUndefined: { attributes: { mathcolor: "red", mathbackground: "#FFEEEE", mathsize: "90%" } } },
#                                    messageStyle: "none"
#                });
#         </script>--><!--
#        <script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS_HTML"></script>-->\n""")

    htmlfile.write("""\n<script src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=default"></script>\n""")
#    htmlfile.write("""
#<script>
#if( !window.MathJax ) {
#    document.write(
#        '<script type="text/javascript" src="../lib/MathJax.js"></script>'
#    );
#}
#</script>
#<!--
#<script>
#var url= location.protocol == 'http:'?
#         "http://cdn.mathjax.org/mathjax/latest/MathJax.js" :
#         "../lib/MathJax.js";
#document.write(
#    '<script type="text/javascript" src="' + url + '"></script>'
#);
#</script>
#-->
#""")

    htmlfile.openTag("center")
    htmlfile.h1(name + " Fit Function", newline=False)
    htmlfile.closeTag(True)

    htmlfile.openTag("p")
    htmlfile.link("wiki help", WEB_BASE+name)
    htmlfile.closeTag(True)
    htmlfile.nl()
    htmlfile.hr()

    htmlfile.h3("Summary")
    wiki = MediaWiki(htmlfile)
    wiki.parse(wiki_tools.get_fitfunc_summary(name, False))
    for img in wiki.images:
        qhp.addFile(os.path.join(HTML_DIR, "img", img))

    if func.numParams() <= 0:
        htmlfile.h3("No Parameters")
    else:
        htmlfile.h3("Parameters")
        htmlfile.openTag("table", {"border":"1", "cellpadding":"5", "cellspacing":"0"})
        htmlfile.writeRow(["Order", "Name", "Default", "Description"], True)
        for i in range(func.numParams()):
            htmlfile.writeRow(propToList(func, i))#[str(i+1), '&nbsp;', '&nbsp;', '&nbsp;'])
        htmlfile.closeTag(True) # close the table

    htmlfile.hr()
    cats = func.categories()
    if len(cats):
        htmlfile.openTag("p")
        htmlfile.write("Categories: ")
        for category in cats:
            ref = "fitfunctions_index.html#%s" % (category)
            htmlfile.write("<a href='%s'>%s</a>" % (ref, category))
        htmlfile.nl()

    # cleanup the file
    htmlfile.closeTag(True)
    htmlfile.closeTag(True)
