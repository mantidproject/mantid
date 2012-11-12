#!/usr/bin/env python
import argparse
from lxml import etree as le # python-lxml on rpm based systems
import lxml.html
from lxml.html import builder as lh
import os
from qhpfile import QHPFile
from string import split,join
import sys

OUTPUTDIR = "generated"
WEB_BASE  = "http://www.mantidproject.org/"

def addWikiDir(helpsrcdir):
    """
    Add the location of the wiki tools to the python path.
    """
    wikitoolsloc = os.path.join(helpsrcdir, "../../Build")
    wikitoolsloc = os.path.abspath(wikitoolsloc)
    sys.path.append(wikitoolsloc)

def genFuncElement(name):
    text = '<a href="%s">%s</a>' % (WEB_BASE+name, name)
    return lxml.html.fragment_fromstring(text)

def processCategories(categories, qhp, outputdir):
    # determine the list of html pages
    grouped_categories = {}
    for key in categories.keys():
        shortkey = key.split('/')[0]
        if not shortkey in grouped_categories:
            grouped_categories[shortkey] = []
        grouped_categories[shortkey].append(key)
    pages = grouped_categories.keys()
    pages.sort()

    for page_name in pages:

        root = le.Element("html")
        head = le.SubElement(root, "head")
        head.append(lh.META(lh.TITLE(page_name + " Algorithm Category")))
        body = le.SubElement(root, "body")
        body.append(lh.CENTER(lh.H1(page_name)))

        subcategories = grouped_categories[page_name]
        subcategories.sort()
        for subcategory in subcategories:
            anchor = subcategory.split('/')
            anchor = '_'.join(anchor[1:])
            temp = le.SubElement(body, "h2")
            le.SubElement(temp, 'a', **{"name":anchor})
            temp.text=subcategory
            #body.append(lh.H2(subcategory))
            ul = le.SubElement(body, "ul")
            for (name, versions) in categories[subcategory]:
                li = le.SubElement(ul, "li")
                li.append(genAlgoElement(name, versions))

        filename = "AlgoCat_%s.html" % page_name
        qhp.addFile(filename, page_name+" Algorithm Category")
        filename = os.path.join(outputdir, filename)
        handle = open(filename, 'w')
        handle.write(le.tostring(root, pretty_print=True, xml_declaration=False))

def process(functionsalgos, qhp, outputdir):
    import mantid.api

    # sort algorithms into categories
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
        div_cat.append(lh.H2(category + " Category"))
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

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate qtassistant docs " \
                  + "for the fit functions")
    defaultmantidpath = ""
    parser.add_argument('-m', '--mantidpath', dest='mantidpath',
                        default=defaultmantidpath,
                        help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)

    args = parser.parse_args()

    # where to put the generated files
    helpsrcdir = os.path.dirname(os.path.abspath(__file__))
    helpoutdir = os.path.join(helpsrcdir, OUTPUTDIR)
    print helpoutdir
    addWikiDir(helpsrcdir)

    # initialize mantid
    import wiki_tools
    wiki_tools.initialize_Mantid(args.mantidpath)
    import mantid.api
    functions = mantid.api.FunctionFactory.getFunctionNames()

    # setup the qhp file
    qhp = QHPFile("org.mantidproject.fitfunctions")

    process(functions, qhp, helpoutdir)
    qhp.write(os.path.join(helpoutdir, "fitfunctions.qhp"))
