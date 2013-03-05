HTML_DIR = "html"
QCH_DIR = "qch"
IMG_DIR = "src/images"
WEB_BASE  = "http://www.mantidproject.org/"

def getParser(description):
    import argparse
    parser = argparse.ArgumentParser(description=description)
    defaultmantidpath = ""
    parser.add_argument('-m', '--mantidpath', dest='mantidpath',
                        default=defaultmantidpath,
                        help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)
    parser.add_argument('-o', '--output', dest='helpoutdir',
                        help="Full path to where the output files should go.")
    return parser

def assertDirs(outputdir, verbose=False):
    import os

    for direc in (HTML_DIR, QCH_DIR, IMG_DIR):
        direc = os.path.join(outputdir, direc)
        if not os.path.exists(direc):
            if verbose:
                print "creating '%s'" % direc
            os.makedirs(direc)
