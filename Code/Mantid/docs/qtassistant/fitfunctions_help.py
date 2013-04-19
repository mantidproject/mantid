import os
from xml.dom.minidom import Document
from assistant_common import WEB_BASE, HTML_DIR, addEle, addTxtEle

def process_function(name, qhp, outputdir, **kwargs): # was (args, algo):
    import mantid.api
    func = mantid.api.FunctionFactory.createFunction(name)
    #print "***", func, dir(func)

    doc = Document()
    root = addEle(doc, "html", doc)
    head = addEle(doc, "head", root)
    addTxtEle(doc, "title", name + " Fit Function", head)
    body = addEle(doc, "body", root)
    temp = addEle(doc, "center", body)
    addTxtEle(doc, "h1", name + " Fit Function", temp)
    addTxtEle(doc, "a", "wiki help", body, {"href":WEB_BASE+name})
    addEle(doc, "hr", body)
    addTxtEle(doc, "h3", "Summary", body)
    if func.numParams() <= 0:
        addTxtEle(doc, "h3", "No Parameters", body)
    else:
        addTxtEle(doc, "h3", "Parameters", body)
        table = addEle(doc, "table", body, {"border":"1", "cellpadding":"5", "cellspacing":"0"})
        header_row = addEle(doc, "tr", table)
        addTxtEle(doc, "th", "Order", header_row)
        addTxtEle(doc, "th", "Name", header_row)
        addTxtEle(doc, "th", "Default", header_row)
        addTxtEle(doc, "th", "Explicit", header_row)
        addTxtEle(doc, "th", "Description", header_row)
        for number in range(func.numParams()):
            row = addEle(doc, "tr", table)
            addTxtEle(doc, "td", str(number+1), row)
            addTxtEle(doc, "td", func.getParamName(number), row)
            addTxtEle(doc, "td", str(func.getParamValue(number)), row)
            addTxtEle(doc, "td", str(func.getParamExplicit(number)), row)
            descr = func.getParamDescr(number)
            if len(descr) <= 0:
                # u"\u00A0".encode('utf-16') # hack to create r'&nbsp;'
                descr = " " # should be &nbsp
            addTxtEle(doc, "td", descr, row)

    cats = []
    for category in func.categories():
        ref = "fitfunctions_index.html#%s" % (category)
        cats.append(addTxtEle(doc, "a", category, attrs={"href":ref}))
    if len(cats) > 0:
        p = addEle(doc, "p", body)
        addTxtEle(doc, "b", "Categories:", p)
        for category in cats:
            p.appendChild(category)

    # write out the file
    outfile = "FitFunc_%s.html" % name
    qhp.addFile(os.path.join(HTML_DIR, outfile), name)
    outfile = os.path.join(outputdir, outfile)
    handle = open(outfile, 'w')
    handle.write(doc.toprettyxml(indent="  ", encoding='utf-8'))
