#!/usr/bin/env python
from lxml import etree as le # python-lxml on rpm based systems
import lxml.html
from lxml.html import builder as lh
import os
from qhpfile import QHPFile
from string import split,join
import sys
from assistant_common import *

def addWikiDir(helpsrcdir):
    """
    Add the location of the wiki tools to the python path.
    """
    wikitoolsloc = os.path.join(helpsrcdir, "../../Build")
    wikitoolsloc = os.path.abspath(wikitoolsloc)
    sys.path.append(wikitoolsloc)

def genFuncElement(name):
    text = '<a href="FitFunc_%s.html">%s</a>' % (name, name)
    return lxml.html.fragment_fromstring(text)

def process(functions, qhp, outputdir):
    import mantid.api

    # sort fit functions into categories
    categories = {}
    for name in functions:
        func = mantid.api.FunctionFactory.createFunction(name)
        for category in func.categories():
            category = category.replace('\\', '/')
            if not categories.has_key(category):
                categories[category] = []
            categories[category].append(name)
    categories_list = categories.keys()
    categories_list.sort()

    ##### put together the top of the html document
    root = le.Element("html")
    head = le.SubElement(root, "head")
    head.append(lh.META(lh.TITLE("Fit Functions Index")))
    body = le.SubElement(root, "body")
    body.append(lh.CENTER(lh.H1("Fit Functions Index")))

    ##### section for categories 
    div_cat = le.SubElement(body, "div", **{"id":"function_cats"})
    for category in categories_list:
        temp = le.SubElement(div_cat, "h2")
        le.SubElement(temp, 'a', **{"name":category})
        temp.text = category + " Category"
        ul = le.SubElement(div_cat, "ul")
        funcs = categories[category]
        for func in funcs:
            li = le.SubElement(ul, "li")
            li.append(genFuncElement(func))

    filename = os.path.join(outputdir, "fitfunctions_index.html")
    handle = open(filename, 'w')
    handle.write(le.tostring(root, pretty_print=True, xml_declaration=False))

    shortname = os.path.split(filename)[1]
    qhp.addFile(shortname, "Fit Functions Index")

    # create individual html pages
    from fitfunctions_help import process_function
    for func in functions:
        process_function(func, qhp, outputdir)

if __name__ == "__main__":
    parser = getParser("Generate qtassistant docs for the fit functions")
    args = parser.parse_args()

    # where to put the generated files
    helpsrcdir = os.path.dirname(os.path.abspath(__file__))
    if args.helpoutdir is not None:
        helpoutdir = os.path.abspath(args.helpoutdir)
    else:
        raise RuntimeError("need to specify output directory")
    print "Writing fit function web pages to '%s'" % helpoutdir
    assertDirs(helpoutdir)
    addWikiDir(helpsrcdir)

    # initialize mantid
    import wiki_tools
    wiki_tools.initialize_Mantid(args.mantidpath)
    import mantid.api
    functions = mantid.api.FunctionFactory.getFunctionNames()

    # setup the qhp file
    qhp = QHPFile("org.mantidproject.fitfunctions")

    process(functions, qhp, os.path.join(helpoutdir, HTML_DIR))
    qhp.write(os.path.join(helpoutdir, "fitfunctions.qhp"))
