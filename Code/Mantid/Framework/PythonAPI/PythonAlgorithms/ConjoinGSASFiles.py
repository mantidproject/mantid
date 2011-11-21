"""*WIKI* 
Conjoin several GSAS files to one single GSAS file.
The Bank will be re-defined 


*WIKI*"""

from MantidFramework import *
from mantidsimple import *
import os

class ConjoinGSASFiles(PythonAlgorithm):
    def category(self):
        """
        """
        return "DataHandling"

    def name(self):
        """
        """
        return "ConjoinGSASFiles"

    def _loadGSASFile(self, filename):
        """ Load GSAS file
        """
        gsasfiledict = {}

        # 1. read file
        gfile = open(filename, "r")
        lines = gfile.readlines()
        gfile.close()

        # 2. put to different section
        stage = "start"
        gsasfiledict["start"] = []
        gsasfiledict["detinfo"] = []
        gsasfiledict["data"] = []
        for line in lines:
            if line.strip() == "":
                continue

            # a) Change of status
            if line.startswith("# Total flight path"):
                # Start detector information line
                stage = "detinfo"
                infolines = []
                gsasfiledict["detinfo"].append(infolines)
            # ENDIF

            if stage == "start":
                # Append start lines
                gsasfiledict["start"].append(line.strip())

            elif stage == "detinfo":
                # Append information  line
                infolines.append(line)

                if line.startswith("BANK"):
                    stage = "data"
                    datalines = []
                    gsasfiledict["data"].append(datalines)

            elif stage == "data":
                # Data lines
                datalines.append(line)
            
            else:
                raise NotImplementedError("")
        # ENDFOR

        return gsasfiledict


    def __load(self, directory, instr, run, loader, exts):
        """ Load GSAS file by run number and etc information
        
        Arguments:
         - directory
         - instr        : Short name for instrument
         - run          : Run number
         - exts         : List of extensions
        """
        for ext in exts:
            # 1. Construct file name
            filename = "%s_%s%s" % (instr, str(run), ext)
            if len(directory) > 0:
                filename = os.path.join(directory, filename)

            if not os.path.exists(filename):
                self.log().information("File %s does not exist" % (filename))
                continue
            else:
                self.log().information("Load File %s" % (filename))
            # END-IF

            # 2. Load file
            wksp = "%s_%s" % (instr, str(run))
            try:
                self.log().information("Trying to load '%s' to Workspace" % filename)
                loader(filename, wksp)
            except Exception, e:
                self.log().error("Failed to load run %s" % str(run))              

            # 3. Load file to string
            gsasfiledict = self._loadGSASFile(filename)
        # ENDFOR

        return wksp, gsasfiledict


    def _writeConjoinedGSASFile(self, filedicts, directory, filename):
        """ Write all banks to a conjoined file
        """
        if not filename.endswith(".gsa"):
            filename += ".gsa"
        if len(directory) > 0:
            filename = os.path.join(directory, filename)
	self.log().information("Output File = %s" % (filename))
	
        if len(filedicts) == 0:
            raise NotImplementedError("Not GSAS file imported")

        outbuffer = ""

        # 1. Header
        for line in filedicts[0]["start"]:
            outbuffer += line + "\n"

        # 2. Body
        bankid = 1
        for filedict in filedicts:
            for bid in xrange(len(filedict["detinfo"])):
                for line in filedict["detinfo"][bid]:
                    if line.strip().startswith("BANK"):
                        # Rename BANK ID
                        terms = line.split()
                        terms[1] = str(bankid)
                        bankid += 1
                        line = ""
                        for term in terms:
                            line += "%s " % (term)
                        line += "\n"
                    # ENDIF
                    outbuffer += line
                for line in filedict["data"][bid]:
                    outbuffer += line

        # 3. Write
        ofile = open(filename, "w")
        ofile.write(outbuffer)
        ofile.close()

        return


    def PyInit(self):
        """ Declare properties
        """
        self.declareListProperty("RunNumbers",[0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareFileProperty("OutputFile","", FileAction.Save, ['.gsa'],
                Description="The file conjoining all input GSAS files")
        self.declareFileProperty("Directory", "", FileAction.OptionalDirectory)

        return

    # def TestExec(self):
    #     """ Main exec
    #     """
    #     # generic stuff for running
    #     # wksp = self.getPropertyValue("OutputWorkspace")
    #     runs = [4866, 4871]
    #     instr = mtd.getSettings().facility().instrument().shortName()
    #     outputfilename = "glued"
    #     directory = "/home/wzz/Projects/Mantid-Project/Tests/Instrument"

    #     # change here if you want something other than gsas files
    #     exts = ['.gsa']
    #     loader = LoadGSS

    #     # load things and conjoin them
    #     filedicts = []
    #     for run in runs:
    #         run = str(run)
    #         wksp, gsasfiledict = self.__load(directory, instr, run, loader, exts)
    #         filedicts.append(gsasfiledict)

    #     self._writeConjoinedGSASFile(filedicts, directory, outputfilename)

    def PyExec(self):
        """ Main exec
        """
        # generic stuff for running
        # wksp = self.getPropertyValue("OutputWorkspace")
        runs = self.getProperty("RunNumbers")
        instr = mtd.getSettings().facility().instrument().shortName()
        outputfilename = self.getPropertyValue("OutputFile")
        directory = self.getPropertyValue("Directory").strip()

        # change here if you want something other than gsas files
        exts = ['.gsa']
        loader = LoadGSS

        # load things and conjoin them
        filedicts = []
        for run in runs:
            run = str(run)
            wksp, gsasfiledict = self.__load(directory, instr, run, loader, exts)
            filedicts.append(gsasfiledict)

        self._writeConjoinedGSASFile(filedicts, directory, outputfilename)

        return

mtd.registerPyAlgorithm(ConjoinGSASFiles())
