import os
from xml.dom.minidom import Document
from assistant_common import WEB_BASE, HTML_DIR, addEle, addTxtEle
from htmlwriter import HtmlWriter

DIRECTION = {
    0:"input",
    1:"output",
    2:"input/output"
}
from assistant_common import WEB_BASE, HTML_DIR

def addCatLink(doc, node, category):
    category = category.replace('\\', '/')
    category = category.split('/')
    filename = 'AlgoCat_' + category[0] + '.html'
    url = filename
    
    if len(category) > 1:
        text = '/'.join(category[:-1]) + '/'
        url += '#' + '_'.join(category[1:])
        text = doc.createTextNode(text)
        node.appendChild(text)
    addTxtEle(doc, "a", category[-1], node, {"href":url})

def writeCatLink(htmlfile, category):
    category = category.replace('\\', '/')
    category = category.split('/')
    filename = 'AlgoCat_' + category[0] + '.html'
    url = filename
    
    if len(category) > 1:
        text = '/'.join(category[:-1]) + '/'
        url += '#' + '_'.join(category[1:])
        htmlfile.write(text)
    htmlfile.link(category[-1], url)

"""
def make_wiki(algo_name, version, latest_version):
     Return wiki text for a given algorithm
    @param algo_name :: name of the algorithm (bare)
    @param version :: version requested
    @param latest_version :: the latest algorithm 
    
    # Deprecated algorithms: Simply returnd the deprecation message
    deprec = mtd.algorithmDeprecationMessage(algo_name,version)
    if len(deprec) != 0:
        out = deprec
        out = out.replace(". Use ", ". Use [[")
        out = out.replace(" instead.", "]] instead.")
        return out
    
    out = ""
    alg = mtd.createAlgorithm(algo_name, version)
    
    if (latest_version > 1):
        if (version < latest_version):
            out += "Note: This page refers to version %d of %s. The latest version is %d - see [[%s v.%d]].\n\n" % (version, algo_name, latest_version, algo_name, latest_version)
        else:
            out += "Note: This page refers to version %d of %s. "% (version, algo_name)
            if latest_version > 2:
                out += "The documentation for older versions is available at: "
            else:
                out += "The documentation for the older version is available at: "
            for v in xrange(1,latest_version):
                out += "[[%s v.%d]] " % (algo_name, v)
            out += "\n\n"
        
    
    out += "== Summary ==\n\n"
    out += alg._ProxyObject__obj.getWikiSummary().replace("\n", " ") + "\n\n"
    out += "== Properties ==\n\n"
    
    out += '''{| border="1" cellpadding="5" cellspacing="0" 
!Order\n!Name\n!Direction\n!Type\n!Default\n!Description
|-\n'''

    # Do all the properties
    props = alg._ProxyObject__obj.getProperties()
    propnum = 1
    last_group = ""
    for prop in props:
        group = prop.getGroup
        if (group != last_group):
            out += make_group_header_line(group)
            last_group = group
        out += make_property_table_line(propnum, prop)
        propnum += 1
        
        
    # Close the table
    out += "|}\n\n"


    out += "== Description ==\n"
    out += "\n"
    desc = get_wiki_description(algo_name,version)
    if (desc == ""):
      out += "INSERT FULL DESCRIPTION HERE\n"
      print "Warning: missing wiki description for %s! Placeholder inserted instead." % algo_name
    else:
      out += desc + "\n"
    out += "\n"
    out += "[[Category:Algorithms]]\n"
    
    # All other categories
    categories = alg.categories()
    for categ in categories:
        n = categ.find("\\")
        if (n>0):
            # Category is "first\second"
            first = categ[0:n]
            second = categ[n+1:]
            out += "[[Category:" + first + "]]\n"
            out += "[[Category:" + second + "]]\n"
        else:
            out += "[[Category:" + categ + "]]\n"

    # Point to the right source ffiles
    if version > 1:
        out +=  "{{AlgorithmLinks|%s%d}}\n" % (algo_name, version)
    else:
        out +=  "{{AlgorithmLinks|%s}}\n" % (algo_name)

    return out
"""

def typeStr(property):
    propType = str(type(property))
    if "FileProperty" in propType:
        return "File"
    if "MatrixWorkspaceProperty" in propType:
        return "MatrixWorkspace"
    if "PropertyWithValue_bool" in propType:
        return "boolean"
    if "PropertyWithValue_int" in propType:
        return "int"
    if "PropertyWithValue_string" in propType:
        return "string"
    if "PropertyWithValue_double" in propType:
        return "double"
    return propType

def propToList(property, number):
    # wiki_maker does this better
    result = [str(number+1)]
    result.append(property.name)
    result.append(DIRECTION[property.direction])
    result.append(typeStr(property))
    result.append('???')
    result.append(property.documentation)
    return result
    """ property methods
    ['__class__', '__delattr__', '__dict__', '__doc__', '__format__', '__getattribute__', '__hash__', '__init__', '__module__', '__new__', '__reduce__', '__reduce_ex__', '__repr__', '__setattr__', '__sizeof__', '__str__', '__subclasshook__', '__weakref__', 'allowedValues', 'direction', 'documentation', 'getGroup', 'isDefault', 'isValid', 'name', 'units', 'value', 'valueAsStr']
    """

def process_algorithm(name, versions, qhp, outputdir, **kwargs): # was (args, algo):
    import mantid
    import wiki_tools

    categories = []
    outfile = "Algo_%s.html" % (name)

    # put newest first
    versions = list(versions)
    versions.reverse()

    htmlfile = HtmlWriter(os.path.join(outputdir, outfile), name)
    htmlfile.openTag("center")
    htmlfile.h1(name, newline=False)
    htmlfile.closeTag(True)

    htmlfile.link("wiki help", WEB_BASE+name)
    htmlfile.nl()
    htmlfile.hr()

    num_versions = len(versions)
    for version in versions:
        htmlfile.openTag("div", {"id":"version_"+str(version)})
        htmlfile.nl()
        if num_versions > 0:
            htmlfile.h2("Version %d" % version)

        alg = mantid.FrameworkManager.createAlgorithm(name, version)
        categories.extend(alg.categories())

        htmlfile.h3("Summary")
        htmlfile.p(alg.getWikiSummary())

        htmlfile.h3("Usage")
        text = wiki_tools.create_function_signature(alg, name)
        htmlfile.openTag("p")
        htmlfile.addTxtEle("code", text)
        htmlfile.closeTag(True)

        htmlfile.h3("Properties")
        htmlfile.openTag("table", {"border":"1", "cellpadding":"5", "cellspacing":"0"})
        htmlfile.writeRow(["Order", "Name", "Direction", "Type", "Default", "Description"], True)
        properties = alg.getProperties()
        mandatory = alg.mandatoryProperties()
        for (i, property) in zip(range(len(properties)), properties):
            htmlfile.writeRow(propToList(property, i))
        htmlfile.closeTag(True)

        htmlfile.h3("Description")
        text = wiki_tools.get_custom_wiki_section(name, version, "*WIKI*", True, False)
        #print name, version, "***", text
        htmlfile.p(text)

        htmlfile.closeTag(True)

        """ algorithm methods
['__class__', '__contains__', '__delattr__', '__dict__', '__doc__', '__format__', '__getattribute__', '__getitem__', '__hash__', '__init__', '__len__', '__module__', '__new__', '__reduce__', '__reduce_ex__', '__repr__', '__setattr__', '__sizeof__', '__str__', '__subclasshook__', '__weakref__', 'alias', 'categories', 'category', 'docString', 'execute', 'existsProperty', 'getOptionalMessage', 'getProperties', 'getProperty', 'getPropertyValue', 'getWikiDescription', 'getWikiSummary', 'initialize', 'isChild', 'isExecuted', 'isInitialized', 'mandatoryProperties', 'name', 'orderedProperties', 'outputProperties', 'propertyCount', 'setAlwaysStoreInADS', 'setChild', 'setLogging', 'setProperty', 'setPropertyValue', 'setRethrows', 'version']
        """

        if version > 1:
            htmlfile.hr()

    # add index for categories
    htmlfile.hr()
    categories = list(set(categories)) # remove duplicates
    htmlfile.openTag("p")
    htmlfile.write("Categories: ")
    for i in range(len(categories)):
        writeCatLink(htmlfile, categories[i])
        if i + 1 < len(categories):
            htmlfile.write(', ')

    # cleanup the file
    htmlfile.nl()
    htmlfile.closeTag(True)
    htmlfile.closeTag(True)


    """ Do the wiki page
    @param algo :: the name of the algorithm, possibly with suffix #
    global mtd
    
    is_latest_version = True
    version = -1;
    latest_version = -1
    if not args.no_version_check:
        if algo.endswith('1'): version = 1
        if algo.endswith('2'): version = 2
        if algo.endswith('3'): version = 3
        if algo.endswith('4'): version = 4
        if algo.endswith('5'): version = 5
        if version > 0:
            algo = algo[:-1]

    # Find the latest version        
    latest_version = mtd.createAlgorithm(algo, -1).version()
    if (version == -1): version = latest_version
    print "Latest version of %s is %d. You are making version %d." % (algo, latest_version, version)
    
    # What should the name on the wiki page be?
    wiki_page_name = algo
    if latest_version > 1:
        wiki_page_name = algo + " v." + str(version)
        # Make sure there is a redirect to latest version
        make_redirect(algo, algo + " v." + str(latest_version))
        
    
    print "Generating wiki page for %s at http://www.mantidproject.org/%s" % (algo, wiki_page_name)
    site = wiki_tools.site
    new_contents = make_wiki(algo, version, latest_version) 
    
    #Open the page with the name of the algo
    page = site.Pages[wiki_page_name]
    
    old_contents = page.edit() + "\n"
    
    if old_contents == new_contents:
        print "Generated wiki page is identical to that on the website."
    else:
        print "Generated wiki page is DIFFERENT than that on the website."
        print
        print "Printing out diff:"
        print
        # Perform a diff of the new vs old contents
        diff = difflib.context_diff(old_contents.splitlines(True), new_contents.splitlines(True), fromfile='website', tofile='new')
        for line in diff:
            sys.stdout.write(line) 
        print
        
        if args.force or confirm("Do you want to replace the website wiki page?", True):
            print "Saving page to http://www.mantidproject.org/%s" % wiki_page_name
            page.save(new_contents, summary = 'Bot: replaced contents using the wiki_maker.py script.' )

    saved_text = open(wiki_page_name+'.txt', 'w')
    saved_text.write(new_contents)
    saved_text.close()
    """
