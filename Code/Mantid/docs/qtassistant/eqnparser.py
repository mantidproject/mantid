"""
This was taken from https://github.com/constantAmateur/scihtmlatex
and modified to be used for generating equations in mantid.
"""
#from BeautifulSoup import BeautifulSoup
import os, os.path 
import subprocess
import tempfile, binascii
from cStringIO import StringIO
import hashlib

DEBUG = 0
TEMPDIR = '/tmp/'
#IMGDIR and IMGURL must exist on the webserver
IMGDIR = '/tmp/htmllatex'#'/home/sciteit/sciteit/r2/r2/public/static/htmlatex/'
IMGURL = '/static/htmlatex/'
LATEX = '/usr/bin/latex'
DVIPNG = '/usr/bin/dvipng'
# Trading size for time-to-generate is a only a good idea when using memcache
DVIPNG_FLAGS = '-T tight -D 120 -z 9 -bg Transparent -o '
TYPES = {'div':'eq', 'div':'numeq', 'div':'alignedeq', 'div':'numalignedeq', 'span':'eq', 'div':'matrix', 'div':'det'}

bad_tags = ["include", "def", "command", "loop", "repeat", "open", "toks", "output",
"input", "catcode", "name", "\\^\\^", "\\every", "\\errhelp", "\\errorstopmode",
"\\scrollmode","\\nonstopmode", "\\batchmode", "\\read", "\\write", "csname",
"\\newhelp", "\\uppercase", "\\lowercase", "\\relax", "\\aftergroup", "\\afterassignment",
"\\expandafter", "\\noexpand", "\\special"]


# Include your favourite LaTeX packages and commands here
# ------ NOTE: please leave \usepackage{preview} as the last package
# ------       it plays a role with dvipng in generating to correct
# ------       offset for inline equations
PREAMBLE = r'''
\documentclass[12pt]{article} 
\usepackage{amsmath}
\usepackage{amsthm}
\usepackage{amssymb}
\usepackage{mathrsfs}
\usepackage{gensymb}
\usepackage{preview}
\pagestyle{empty} 
\batchmode
\begin{document} 
'''

def main(data):
    """
    Get the latex source; return a raw string for the webserver to send to the client

    Take the outgoing HTML and turn it into a python object.  Search the object for the
    <div> and <span> tags that indicate latex source -- the tags we can handle are given
    in the global variable TYPES.  If there isn't anything to handle, return.  If there is, 
    process it and return a pretty string of HTML for the webserver to send to the client.
 
    Notes:  A working directory must be set via os.chdir so latex knows where to put its output
    """

    if not os.path.isdir(TEMPDIR):
        os.mkdir(TEMPDIR, 0755)
    os.chdir(TEMPDIR)
    
    if not os.path.isdir(IMGDIR):
        os.mkdir(IMGDIR, 0755)
        for i in ['0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f']:
            os.mkdir(IMGDIR + i, 0755)

    soup = BeautifulSoup(data)

    equations = soup.findAll(TYPES)
    if not equations:
        return data

    for equation in equations:
        key = hashlib.md5()
        key.update(str(equation).strip())                   # Strip to minimize gratuitous differences in keys
        eq = Equation(key.hexdigest(), equation)
        equation.replaceWith(eq.contents[0])
        del(eq)

    return StringIO(soup.prettify()).getvalue()


class Equation:

    def __init__(self, equation):
        """
        Parameters:  key --  an md5 sum of the parsed equation node
                     equation -- the parsed equation node
        Methods:     contents -- base64 encoded PNG

        When an Equation object is initialized, it checks the filesystem
	indiciated by IMGDIR to see if a file with the same name as its
        key is stored; if the key-named file exists, an <img src> tag
        referring to the filesystem is inserted into Equation.contents.

        If it isn't found, the equation's latex source is sanitized, compiled
        to a DVI, turned into a PNG, saved to IMGDIR, and stored in 
	Equation.contents
        """
        self.contents = equation.strip()

        key = hashlib.md5()
        key.update(eqn)
        self.key = key.hexdigest()

        self.eqstring, self.fd = None, None
        self.texfilename, self.texfile, self.dvifile = None, None, None
        self.pngfile = "/tmp/" + self.key + '.png'
        self.pngurl = IMGURL + self.key[0] + '/' + self.key + '.png'
        #self.comment = "\n<!-- %s -->\n" % (self.contents.string.strip())
        self.cached = os.path.isfile(self.pngfile)
        if self.cached:
            self.contents = ['<img src="%s" />' % (self.pngurl)]
        else:
            print "generating image file"
            self._addToTex()
            self._compileDVI()
            self._createPNG()
            self.contents = ['<img src="%s" />' % (self.pngurl)]
        
        
    def _addToTex(self):
        """
        Creates a Tex file, writes the preamble and the equation, closed the Tex file.
        
        Calls _translateToTex
        """
        print "00:"
        (self.fd, self.texfilename) = tempfile.mkstemp(suffix='.tex', prefix='eq_', dir=TEMPDIR, text=True)
        print "01:"
        self.texfile = os.fdopen(self.fd, 'w+')
        print "02:"
        self.texfile.write(PREAMBLE)
        print "03:"

        #self._translateToTex()
        self.eqstring = "$ %s $ \\newpage" % self.contents
        self._sanitize()

        print "04:"
        self.texfile.write('\n' + self.eqstring + '\n')
        print "05:"
        self.texfile.write("\n \\end{document}\n")
        print "06:"
        self.texfile.close()
        print "07:"
        return


    def _translateToTex(self):
        """
        Translates the HTML into equivalent latex.
        FIXME: This is getting a little unwieldly
        
        Calls _sanitize
        """
        if self.contents.name == 'span':
            if self.contents.attrs[0][1] == 'eq':
                self.eqstring = "$ %s $ \\newpage" % self.contents.string.strip()
                self._sanitize()
                return
            elif self.contents.attrs[0][1] == 'det':
                self.eqstring = "\\begin{equation*} \\left| \\begin{matrix} %s \\end{matrix} \\right| \\end{equation*}" % \
                self.contents.string.strip()
                return
            elif self.contents.attrs[0][1] == 'matrix':
                self.eqstring = "\\begin{equation*} \\left[ \\begin{matrix} %s \\end{matrix} \\right] \\end{equation*}" % \
                self.contents.string.strip()
                return
            if DEBUG:
                assert False, 'Unhandled span:  %s at %s' % (self.eqstring)
            return
        elif self.contents.name == 'div':
            if self.contents.attrs[0][1] == 'matrix':
                self.eqstring = "\\begin{equation*} \\left[ \\begin{matrix} %s \\end{matrix} \\right] \\end{equation*}" % \
                self.contents.string.strip()
                self._sanitize()
                return
            elif self.contents.attrs[0][1] == 'det':
                self.eqstring = "\\begin{equation*} \\left| \\begin{matrix} %s \\end{matrix} \\right| \\end{equation*}" % \
                self.contents.string.strip()
                self._sanitize()
                return
            elif self.contents.attrs[0][1] == 'alignedeq':
                self.eqstring = "\\begin{align*} %s \\end{align*}" % \
                self.contents.string.strip()
                self._sanitize()
                return
            elif self.contents.attrs[0][1] == 'numalignedeq':
                self.eqstring = "\\begin{align} %s \\end{align}" % \
                self.contents.string.strip()
                self._sanitize()
                return
            elif self.contents.attrs[0][1] == 'numeq':
                self.eqstring = "\\begin{equation} %s \\end{equation}" % \
                self.contents.string.strip()
                self._sanitize()
                return
            else:
                self.eqstring = "\\begin{equation*} %s \\end{equation*}" % \
                self.contents.string.strip()
                self._sanitize()
                return
            if DEBUG:
                assert False, 'Unhandled div:  %s at %s' % (self.eqstring)
            return


    def _sanitize(self):
        """
        Removes potentially dangerous latex code, replacing it with
        a 'LaTeX sanitized' message
        """
        for tag in bad_tags:
            if tag in self.eqstring.lower():
                self.eqstring = "$ \\mbox{\\LaTeX sanitized} $ \\newpage"
        return 


    def _compileDVI(self):
        """
        Compiles the Tex file into a DVI.  If there's an error, raise it.
        """
        if not os.path.exists(LATEX):
            raise RuntimeError("latex executable ('%s') does not exist" % LATEX)

        #os.spawnl(os.P_WAIT, LATEX, 'latex', self.texfilename)
        #retcode = subprocess.call([LATEX, self.texfilename])
        proc = subprocess.Popen([LATEX, self.texfilename], cwd="/tmp")
        retcode = proc.wait()
        print "latex returned %d" % retcode
	#Open the log file and see if anything went wrong
	f=open(self.texfilename[:-3]+'log')
	for line in f:
		if line[0]=="!":
			raise SyntaxError(line)
	f.close()
        if not DEBUG:
                os.remove(self.texfilename)
                os.remove(self.texfilename[:-3] + 'aux')
                os.remove(self.texfilename[:-3] + 'log')
        return 

    def _createPNG(self):
        """
        Runs dvipng on the DVI file.
        Encodes the original latex as an HTML comment.
        """
        if not os.path.exists(DVIPNG):
            raise RuntimeError("dvipng executable ('%s') does not exist" % DVIPNG)

        cmd = '%s %s "%s" %s 2>/dev/null 1>/dev/null' % (DVIPNG, DVIPNG_FLAGS, self.pngfile, self.dvifile)
        proc = subprocess.Popen([DVIPNG, DVIPNG_FLAGS, '"%"' % self.pngfile, self.dvifile],
                                cwd="/tmp")
        retcode = proc.wait()
        print "dvipng returned %d" % retcode

        self.dvifile = self.texfilename[:-3] + 'dvi'
        #os.spawnl(os.P_WAIT, DVIPNG, 'dvipng', '%s %s %s' % (DVIPNG_FLAGS, self.pngfile, self.dvifile))
        os.system('%s %s "%s" %s 2>/dev/null 1>/dev/null' % (DVIPNG, DVIPNG_FLAGS, self.pngfile, self.dvifile))
	if not DEBUG:
            os.remove(self.dvifile)
        return

if __name__ == "__main__":
    import sys
    infile = sys.argv[1]
    outfile = infile + ".new"
    print infile, "->", outfile

    text = file(infile,'r').read()
    start = text.find("<math>")
    stop = text.find("</math>", start)
    eqn = text[start+6:stop]
    print eqn
    key = hashlib.md5()
    key.update(eqn)

    eqn = Equation(eqn)
    print "AAA:", eqn
    print "BBB:", eqn.contents
