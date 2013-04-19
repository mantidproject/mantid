HTML_DIR = "html"
QCH_DIR = "qch"
IMG_DIR = "src/images"
WEB_BASE  = "http://www.mantidproject.org/"

def getParser(description):
    import optparse
    parser = optparse.OptionParser(description=description)
    defaultmantidpath = ""
    parser.add_option('-m', '--mantidpath', dest='mantidpath',
                      default=defaultmantidpath,
                      help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)
    parser.add_option('-o', '--output', dest='helpoutdir',
                      help="Full path to where the output files should go.")
    return parser

def assertDirs(outputdir, verbose=False):
    import os

    for direc in (HTML_DIR, QCH_DIR, IMG_DIR):
        direc = os.path.join(outputdir, direc)
        direc = os.path.abspath(direc)
        if not os.path.exists(direc):
            if verbose:
                print "creating '%s'" % direc
            try:
                os.makedirs(direc)
            except OSError, e:
                # EEXIST says that the file already exists
                if e.errno != os.errno.EEXIST:
                    raise e

def addEle(doc, tag, parent=None, attrs={}):
    """Assumes that the 'doc' that comes in is a xml.dom.minidom.Document
    """
    ele = doc.createElement(tag)
    for key in attrs.keys():
        ele.setAttribute(key, attrs[key])
    if parent is not None:
        parent.appendChild(ele)
    return ele

def addTxtEle(doc, tag, text, parent=None, attrs={}):
    """Assumes that the 'doc' that comes in is a xml.dom.minidom.Document
    """
    ele = addEle(doc, tag, parent, attrs)
    text = doc.createTextNode(text)
    ele.appendChild(text)
    return ele
