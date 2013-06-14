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

def addLetterIndex(doc, div, letters):
    para = addEle(doc, "p", div)
    for letter in map(chr, range(65, 91)):
        if letter in letters:
            addTxtEle(doc, "a", letter, para, {"href":'#algo%s' % letter})
            text = doc.createTextNode(" ")
            para.appendChild(text)
        else:
            text = doc.createTextNode(letter+" ")
            para.appendChild(text)


def appendCatElement(doc, ul, category):
    category = category.split('/')
    filename = 'AlgoCat_' + category[0] + '.html'
    url = filename
    
    li = addEle(doc, "li", ul)

    if len(category) > 1:
        text = '/'.join(category[:-1]) + '/'
        url += '#' + '_'.join(category[1:])
        text = doc.createTextNode(text)
        li.appendChild(text)
    addTxtEle(doc, "a", category[-1], li, {"href":url})

def appendAlgoElement(doc, ul, name, versions):
    li = addEle(doc, "li", ul)
    writeVersions = True
    try:
        url = 'Algo_%s.html' % (name)
        text = "%s v%d" % (name, versions[-1])
    except TypeError, e:
        if versions.startswith("ALIAS:"):
            (temp, real) = versions.split(":")
            text = name
            url = 'Algo_%s.html' % (real)
            writeVersions = False
        else:
            raise e
    addTxtEle(doc, "a", text, li, {'href':url})

    if writeVersions and len(versions) > 1:
        text = ''
        text += ', ' + ', '.join(['v'+str(version) for version in versions[:-1]])
        text = doc.createTextNode(text)
        li.appendChild(text)

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
        doc = Document()
        root = addEle(doc, "html", doc)
        head = addEle(doc, "head", root)
        addTxtEle(doc, "title", page_name + " Algorithm Category", head)
        body = addEle(doc, "body", root)
        temp = addEle(doc, "center", body)
        addTxtEle(doc, "h1", page_name, temp)

        subcategories = grouped_categories[page_name]
        subcategories.sort()
        for subcategory in subcategories:
            anchor = subcategory.split('/')
            anchor = '_'.join(anchor[1:])
            addTxtEle(doc, "h2", subcategory, body)
            addEle(doc, 'a', body, {"name":anchor})
            ul = addEle(doc, "ul", body)
            for (name, versions) in categories[subcategory]:
                appendAlgoElement(doc, ul, name, versions)

        filename = "AlgoCat_%s.html" % page_name
        qhp.addFile(os.path.join(HTML_DIR, filename), page_name)
        filename = os.path.join(outputdir, filename)
        handle = open(filename, 'w')
        handle.write(doc.toxml(encoding="utf-8"))

def process(algos, qhp, outputdir, options):
    import mantid

    # sort algorithms into categories
    categories = {}
    for name in algos.keys():
        versions = algos[name]

        alg = mantid.FrameworkManager.createAlgorithm(name, versions[-1])
        alias = alg.alias().strip()
        alg_categories = alg.categories()
        try:
            alg_categories = alg_categories.split(';')
        except AttributeError, e:
            pass # the categories are already a list
        for category in alg_categories:
            category = category.replace('\\', '/')
            if not categories.has_key(category):
                categories[category] = []
            categories[category].append((name, versions))
            if len(alias) > 0:
                categories[category].append((alias, "ALIAS:"+name))
    categories_list = categories.keys()
    categories_list.sort()

    # sort algorithms into letter based groups
    letter_groups = {}
    for name in algos.keys():
        versions = algos[name]
        letter = str(name)[0].upper()
        if not letter_groups.has_key(letter):
            letter_groups[letter] = []
        letter_groups[letter].append((str(name), versions))

        # add in the alias
        alias = mantid.FrameworkManager.createAlgorithm(name, versions[-1]).alias().strip()
        if len(alias) > 0:
            letter = str(name)[0].upper()
            if not letter_groups.has_key(letter):
                letter_groups[letter] = []
            letter_groups[letter].append((alias, "ALIAS:"+name))

    ##### put together the top of the html document
    doc = Document()
    root = addEle(doc, "html", doc)
    head = addEle(doc, "head", root)
    addTxtEle(doc, "title", "Algorithms Index", head)
    body = addEle(doc, "body", root)
    temp = addEle(doc, "center", body)
    addTxtEle(doc, "h1", "Algorithms Index", temp)

    ##### section for categories 
    div_cat = addEle(doc, "div", body, {"id":"alg_cats"})
    addTxtEle(doc, "h2", "Subcategories", div_cat)
    above = None
    ul = addEle(doc, "ul", div_cat)
    for category in categories_list:
        appendCatElement(doc, ul, category)

    ##### section for alphabetical 
    div_alpha = addEle(doc, "div", body, {"id":"alg_alpha"})
    addTxtEle(doc, "h2", "Alphabetical", div_alpha)

    letters = letter_groups.keys()
    letters.sort()

    # print an index within the page
    addLetterIndex(doc, div_alpha, letters)

    # print the list of algorithms by name
    for letter in letters:
        addTxtEle(doc, 'h3', letter, div_alpha)
        addEle(doc, 'a', div_alpha, {"name":'algo'+letter})
        ul = addEle(doc, "ul", div_alpha)
        group = letter_groups[letter]
        group.sort()
        for (name, versions) in group:
            appendAlgoElement(doc, ul, name, versions)

    # print an index within the page
    addLetterIndex(doc, div_alpha, letters)

    filename = os.path.join(outputdir, "algorithms_index.html")
    handle = open(filename, 'w')
    handle.write(doc.toxml(encoding="utf-8"))

    shortname = os.path.split(filename)[1]
    qhp.addFile(os.path.join(HTML_DIR, shortname), "Algorithms Index")

    # create all of the category pages
    processCategories(categories, qhp, outputdir)

    # create individual html pages
    from algorithm_help import process_algorithm
    for name in algos.keys():
        versions = algos[name]
        process_algorithm(name, versions, qhp, outputdir, latex=options.latex, dvipng=options.dvipng, fetchimages=options.fetchimages)

if __name__ == "__main__":
    parser = getParser("Generate qtassistant docs for the algorithms")
    parser.add_option("-g", '--getimages', help="Download algorithm dialog images", default=False, action="store_true", dest="fetchimages")
    (options, args) = parser.parse_args()

    # where to put the generated files
    helpsrcdir = os.path.dirname(os.path.abspath(__file__))
    if options.helpoutdir is not None:
        helpoutdir = os.path.abspath(options.helpoutdir)
    else:
        raise RuntimeError("need to specify output directory")
    print "Writing algorithm web pages to '%s'" % helpoutdir
    assertDirs(helpoutdir)
    addWikiDir(helpsrcdir)

    # initialize mantid
    sys.path.insert(0, options.mantidpath)
    os.environ['MANTIDPATH'] = options.mantidpath
    import mantid.api
    algos = mantid.api.AlgorithmFactory.getRegisteredAlgorithms(True)

    # setup the qhp file
    qhp = QHPFile("org.mantidproject.algorithms")

    process(algos, qhp, os.path.join(helpoutdir, HTML_DIR), options)
    qhp.write(os.path.join(helpoutdir, "algorithms.qhp"))
