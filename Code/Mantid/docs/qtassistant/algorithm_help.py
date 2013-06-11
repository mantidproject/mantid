from assistant_common import WEB_BASE, HTML_DIR
from htmlwriter import HtmlWriter
import mantid
from mediawiki import MediaWiki
import os
import wiki_tools
from parseLinks import fixLinks

DIRECTION = {
    0:"input",
    1:"output",
    2:"input/output"
}
from assistant_common import WEB_BASE, HTML_DIR

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

def propToList(property, number):
    # order
    result = [str(number+1)]
    # name
    result.append(property.name)
    # direction
    result.append(DIRECTION[property.direction])
    # type
    result.append(property.type)
    # default
    if (property.direction == 1) and (not isinstance(property, mantid.api.IWorkspaceProperty)): # 1=direction[output]
        # Nothing to show under the default section for an output properties that are not workspace properties.
        result.append("&nbsp;")
    elif (property.isValid == ""): #Nothing was set, but it's still valid = NOT  mandatory
      defaultstr = wiki_tools.create_property_default_string(property)
      result.append(defaultstr)
    else:
      result.append("mandatory")
    # description
    text = property.documentation
    if len(text) <= 0:
        text = "&nbsp;"
    else:
        text = text.replace("\n", "<br/>")
    #fix links
    fixer=fixLinks(text)
    text = fixer.parse()
    result.append(text)
    return result

def process_algorithm(name, versions, qhp, outputdir, fetchimages, **kwargs): # was (args, algo):
    categories = []
    outfile = "Algo_%s.html" % (name)
    qhp.addFile(os.path.join(HTML_DIR, outfile), name)

    # put newest first
    versions = list(versions)
    versions.reverse()

    htmlfile = HtmlWriter(os.path.join(outputdir, outfile), name)
    htmlfile.openTag("center")
    htmlfile.h1(name, newline=False)
    htmlfile.closeTag(True)

    htmlfile.openTag("p")
    htmlfile.link("wiki help", WEB_BASE+name)
    htmlfile.closeTag(True)
    htmlfile.nl()
    htmlfile.hr()
    
    imgpath = ""
    IMG = "img"
    imagefile = '%s_dlg.png' %name
    
    if fetchimages:
        # os.environ['http_proxy']="http://wwwcache.rl.ac.uk:8080"  #TODO should be cmake variable
        try:
            # Download image
            import urllib
            fileurl = "http://download.mantidproject.org/algorithm_screenshots/ScreenShotImages/%s" % imagefile #TODO location should be cmake variable
            imgpath = os.path.join(HTML_DIR, IMG, imagefile)
            destname = os.path.join(outputdir, IMG, imagefile)
            urllib.urlretrieve(fileurl, filename=destname)   
            
            # Now link to image
            sourcepath = "%s/%s" % (IMG, imagefile)
    
            htmlfile.openTag("img", {"src": sourcepath, "style":"position:relative; z-index:1000; padding-left:5px;", "width":"400", "align":"right"})
            htmlfile.closeTag(True)
            if imgpath != "":
                qhp.addFile(imgpath)
            
        except IOError:
            pass    
    
    num_versions = len(versions)
    for version in versions:
        htmlfile.openTag("div", {"id":"version_"+str(version)})
        htmlfile.nl()
        if num_versions > 0:
            htmlfile.h2("Version %d" % version)

        alg = mantid.FrameworkManager.createAlgorithm(name, version)
        categories.extend(alg.categories())
        
        htmlfile.h3("Summary")
        #htmlfile.p(alg.getWikiSummary())
        wiki = MediaWiki(htmlfile, HTML_DIR, latex=kwargs["latex"], dvipng=kwargs["dvipng"])
        wiki.parse(alg.getWikiSummary(), qhp)

        htmlfile.h3("Usage")
        text = wiki_tools.create_function_signature(alg, name)
        text = text.split("\n")
        if len(text) > 1:
            text[0] = text[0] + "<pre>"
            text[-1] = text[-1] + "</pre>"
        text = "\n".join(text)
        htmlfile.openTag("p")
        htmlfile.addTxtEle("code", text)
        htmlfile.closeTag(True)

        alias = alg.alias().strip()
        if len(alias) > 0:
            htmlfile.h3("Alias")
            htmlfile.p("This algorithm is also called '%s'" % alias)
            qhp.addFile(os.path.join(HTML_DIR, outfile), alias)

        htmlfile.h3("Properties")
        htmlfile.openTag("table", {"border":"1", "cellpadding":"5", "cellspacing":"0"})
        htmlfile.writeRow(["Order", "Name", "Direction", "Type", "Default", "Description"], True)
        properties = alg.getProperties()
        mandatory = alg.mandatoryProperties()
        for (i, property) in zip(range(len(properties)), properties):
            htmlfile.writeRow(propToList(property, i))
        htmlfile.closeTag(True)

        wiki = MediaWiki(htmlfile, HTML_DIR, latex=kwargs["latex"], dvipng=kwargs["dvipng"])
        text = wiki_tools.get_custom_wiki_section(name, version, "*WIKI*", True, False)
        if len(text.strip()) > 0:
            htmlfile.h3("Description")
            wiki.parse(text, qhp)

        htmlfile.closeTag(True)

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
    htmlfile.closeTag(True)
