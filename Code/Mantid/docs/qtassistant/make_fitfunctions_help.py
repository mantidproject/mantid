#!/usr/bin/env python
from xml.dom.minidom import Document
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

def appendFuncElement(doc, div, name):
    li = addEle(doc, "li", div)
    addTxtEle(doc, "a", name, li, {"href":"FitFunc_%s.html" % name})

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
    doc = Document()
    root = addEle(doc, "html", doc)
    head = addEle(doc, "head", root)
    addTxtEle(doc, "title", "Fit Functions Index", head)
    body = addEle(doc, "body", root)
    temp = addEle(doc, "center", body)
    addTxtEle(doc, "h1", "Fit Functions Index", temp)

    ##### section for categories 
    div_cat = addEle(doc, "div", body, {"id":"function_cats"})
    for category in categories_list:
        addTxtEle(doc, "h2", category + " Category", div_cat)
        addEle(doc, "a", div_cat, {"name":category})
        ul = addEle(doc, "ul", div_cat)
        funcs = categories[category]
        for func in funcs:
            appendFuncElement(doc, ul, func)

    filename = os.path.join(outputdir, "fitfunctions_index.html")
    handle = open(filename, 'w')
    handle.write(doc.toprettyxml(indent="  ", encoding="utf-8"))

    shortname = os.path.split(filename)[1]
    qhp.addFile(os.path.join(HTML_DIR, shortname), "Fit Functions Index")

    # create individual html pages
    from fitfunctions_help import process_function
    for func in functions:
        process_function(func, qhp, outputdir)

if __name__ == "__main__":
    parser = getParser("Generate qtassistant docs for the fit functions")
    (options, args) = parser.parse_args()

    # where to put the generated files
    helpsrcdir = os.path.dirname(os.path.abspath(__file__))
    if options.helpoutdir is not None:
        helpoutdir = os.path.abspath(options.helpoutdir)
    else:
        raise RuntimeError("need to specify output directory")
    print "Writing fit function web pages to '%s'" % helpoutdir
    assertDirs(helpoutdir)
    addWikiDir(helpsrcdir)

    # initialize mantid
    sys.path.append(options.mantidpath)
    os.environ['MANTIDPATH'] = options.mantidpath
    import mantid.api
    functions = mantid.api.FunctionFactory.getFunctionNames()

    # setup the qhp file
    qhp = QHPFile("org.mantidproject.fitfunctions")

    process(functions, qhp, os.path.join(helpoutdir, HTML_DIR))
    qhp.write(os.path.join(helpoutdir, "fitfunctions.qhp"))
