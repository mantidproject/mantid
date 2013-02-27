from lxml import etree as le # python-lxml on rpm based systems
import lxml.html
from lxml.html import builder as lhbuilder
import os

WEB_BASE  = "http://www.mantidproject.org/"

def process_function(name, qhp, outputdir, **kwargs): # was (args, algo):
    import mantid.api
    func = mantid.api.FunctionFactory.createFunction(name)
    #print "***", func, dir(func)

    root = le.Element("html")
    head = le.SubElement(root, "head")
    head.append(lhbuilder.META(lhbuilder.TITLE(name + " Fit Function")))
    body = le.SubElement(root, "body")
    body.append(lhbuilder.CENTER(lhbuilder.H1(name + " Fit Function")))
    text = '<a href="%s">wiki help</a>' % (WEB_BASE+name)
    body.append(lxml.html.fragment_fromstring(text))
    body.append(lhbuilder.HR())

    body.append(lhbuilder.H3("Summary"))
    
    body.append(lhbuilder.H3("Properties"))
    table = le.SubElement(body, "table",
                          **{"border":"1", "cellpadding":"5", "cellspacing":"0"})
    header_row = le.SubElement(table, "tr")
    header_row.append(lhbuilder.TH("Order"))
    header_row.append(lhbuilder.TH("Name"))
    header_row.append(lhbuilder.TH("Default"))
    header_row.append(lhbuilder.TH("Description"))
    # TODO expose getting function properties to python so the table can be generated
    #properties = func.getProperties()
    #for (i, property) in zip(range(len(properties)), properties):
    #    propToHtml(table, property, i)

    cats = []
    for category in func.categories():
        cats.append('<a href="fitfunctions_index.html#%s">%s</a>' % (category, category))
    if len(cats) > 0:
        text = '<p><b>Categories:</b> ' + " ".join(cats) + '</p>'
        body.append(lxml.html.fragment_fromstring(text))

    # write out the file
    outfile = "FitFunc_%s.html" % name
    qhp.addFile(outfile, name)
    outfile = os.path.join(outputdir, outfile)
    handle = open(outfile, 'w')
    handle.write(le.tostring(root, pretty_print=True,
                             xml_declaration=False))
