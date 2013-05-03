"""
This was taken from https://github.com/constantAmateur/scihtmlatex
and modified to be used for generating equations in mantid.
"""
#from BeautifulSoup import BeautifulSoup
import hashlib
import logging
import os
import subprocess
import tempfile

LATEX = '/usr/bin/latex'
DVIPNG = '/usr/bin/dvipng'
# Trading size for time-to-generate is a only a good idea when using memcache
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

def canUse():
    """
    This is a method to invoke in case you want to try actually using 
    this chunk of code. True means the dependencies were found.
    """
    if not os.path.exists(LATEX):
        return False
    if not os.path.exists(DVIPNG):
        return False

class Equation:
    def __init__(self, equation, outdir="/tmp", tmpdir="/tmp", urlprefix="img", debug=False):
        """
        When an Equation object is initialized, it checks the filesystem
	indiciated by IMGDIR to see if a file with the same name as its
        key is stored; if the key-named file exists, an <img src> tag
        referring to the filesystem is inserted into Equation.contents.

        If it isn't found, the equation's latex source is sanitized, compiled
        to a DVI, turned into a PNG, saved to IMGDIR, and stored in 
	Equation.contents
        """
        # generate a logger
        self._logger = logging.getLogger(__name__+'.'+self.__class__.__name__)

        # trim out extraneous whitespace
        self.contents = equation.strip()

        # cache some of the other bits of information
        self._debug = debug
        self._filestodelete = []

        # generate the md5 sum
        key = hashlib.md5()
        key.update(self.contents)
        self._key = key.hexdigest()

        # name of the png file and html tag
        self.pngfile = os.path.join(outdir, "eqn_" + self._key + '.png')
        self.contentshtml = '<img src="%s"/>' % (self.pngfile)

        self.cached = os.path.isfile(self.pngfile)
        if not self.cached:
            self._logger.info("Cache file '%s' not found. Generating it." % self.pngfile)
            self._addToTex(tmpdir)
            self._compileDVI()
            self._createPNG()

    def __del__(self):
        # cleanup
        if not self._debug:
            for filename in self._filestodelete:
                if os.path.exists(filename):
                    self._logger.debug("deleting '%s'" % filename)
                    os.remove(filename)
        
    def __repr__(self):
        return self.contents
        
    def _addToTex(self, tmpdir):
        """
        Creates a Tex file, writes the preamble and the equation, closed the Tex file.
        
        Calls _translateToTex
        """
        # create the equation string
        #self._translateToTex() # could be reworked to do this
        self.eqstring = "$ %s $ \\newpage" % self.contents
        self._sanitize()

        # generate the temp file and write out the tex
        (fd, self.texfile) = tempfile.mkstemp(suffix='.tex', prefix='eqn_', dir=tmpdir, text=True)
        self._logger.info("Generating tex file '%s')" % self.texfile)
        handle = os.fdopen(fd, 'w+')
        handle.write(PREAMBLE)
        handle.write('\n' + self.eqstring + '\n')
        handle.write("\n \\end{document}\n")
        handle.close()

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
            elif self.contents.attrs[0][1] == 'det':
                self.eqstring = "\\begin{equation*} \\left| \\begin{matrix} %s \\end{matrix} \\right| \\end{equation*}" % \
                    self.contents.string.strip()
            elif self.contents.attrs[0][1] == 'matrix':
                self.eqstring = "\\begin{equation*} \\left[ \\begin{matrix} %s \\end{matrix} \\right] \\end{equation*}" % \
                    self.contents.string.strip()
            elif self._debug:
                assert False, 'Unhandled span:  %s at %s' % (self.eqstring)
        elif self.contents.name == 'div':
            if self.contents.attrs[0][1] == 'matrix':
                self.eqstring = "\\begin{equation*} \\left[ \\begin{matrix} %s \\end{matrix} \\right] \\end{equation*}" % \
                    self.contents.string.strip()
            elif self.contents.attrs[0][1] == 'det':
                self.eqstring = "\\begin{equation*} \\left| \\begin{matrix} %s \\end{matrix} \\right| \\end{equation*}" % \
                    self.contents.string.strip()
            elif self.contents.attrs[0][1] == 'alignedeq':
                self.eqstring = "\\begin{align*} %s \\end{align*}" % \
                    self.contents.string.strip()
            elif self.contents.attrs[0][1] == 'numalignedeq':
                self.eqstring = "\\begin{align} %s \\end{align}" % \
                    self.contents.string.strip()
            elif self.contents.attrs[0][1] == 'numeq':
                self.eqstring = "\\begin{equation} %s \\end{equation}" % \
                    self.contents.string.strip()
            else:
                self.eqstring = "\\begin{equation*} %s \\end{equation*}" % \
                    self.contents.string.strip()
            self._sanitize()

    def _sanitize(self):
        """
        Removes potentially dangerous latex code, replacing it with
        a 'LaTeX sanitized' message
        """
        lowercase = self.eqstring.lower()
        for tag in bad_tags:
            if tag in lowercase:
                self.eqstring = "$ \\mbox{\\LaTeX sanitized} $ \\newpage"
        return 


    def _compileDVI(self):
        """
        Compiles the Tex file into a DVI.  If there's an error, raise it.
        """
        self._logger.info("Generating dvi file")
        if not os.path.exists(LATEX):
            raise RuntimeError("latex executable ('%s') does not exist" % LATEX)

        # run latex
        cmd = [LATEX, self.texfile]
        self._logger.debug("cmd: '%s'" % " ".join(cmd))
        proc = subprocess.Popen(cmd, cwd="/tmp")
        retcode = proc.wait()
        if retcode != 0:
            raise RuntimeError("'%s' returned %d" % (" ".join(cmd), retcode))

        # names of generated files
	self.dvifile=self.texfile[:-3]+'dvi'
        auxfile = self.texfile[:-3] + 'aux'
        logfile = self.texfile[:-3] + 'log'

	#Open the log file and see if anything went wrong
	handle=open(logfile)
	for line in handle:
		if line[0]=="!":
			raise SyntaxError(line)
	handle.close()

        # register files to delete
        self._filestodelete.append(self.texfile)
        self._filestodelete.append(auxfile)
        self._filestodelete.append(logfile)

    def _createPNG(self):
        """
        Runs dvipng on the DVI file.
        Encodes the original latex as an HTML comment.
        """
        self._logger.info("Generating png file")
        if not os.path.exists(DVIPNG):
            raise RuntimeError("dvipng executable ('%s') does not exist" % DVIPNG)

        cmd = [DVIPNG, '-T tight -D 120 -z 9 -bg Transparent',
               '-o', os.path.split(self.pngfile)[-1],
               os.path.split(self.dvifile)[-1]]

        self._logger.debug("cmd: '%s'" % " ".join(cmd))
        proc = subprocess.Popen(cmd, cwd="/tmp")
        retcode = proc.wait()
        if retcode != 0:
            raise RuntimeError("'%s' returned %d" % (" ".join(cmd), retcode))

        # register files to delete
        self._filestodelete.append(self.dvifile)

if __name__ == "__main__":
    import sys
    infile = sys.argv[1]
    outfile = infile + ".new"
    print infile, "->", outfile

    #logging.basicConfig(level=logging.DEBUG)

    text = file(infile,'r').read()

    start = 0
    while start >= 0:
        start = text.find("<math>", start)
        if start < 0:
            break
        stop = text.find("</math>", start)
        if stop < start:
            break
        orig = text[start+6:stop]
        start = stop
        eqn = Equation(orig)
        text = text.replace("<math>" + orig + "</math>", eqn.contentshtml)

    print "******************************"
    handle = open(outfile, 'w')
    handle.write(text)
    handle.close()
    
