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
        return "DataHandling;PythonAlgorithms"

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


    def __loadGSASFile(self, filename, workspacename, loader):
        """ Load GSAS file 

        Arguments:
         - filename
         - loader
        """
        # 1. Construct file name
        if not os.path.exists(filename):
            self.log().information("File %s does not exist" % (filename))
            return
        else:
            self.log().information("Load File %s" % (filename))
        # END-IF

        # 2. Load file
        try:
            self.log().information("Trying to load '%s' to Workspace" % filename)
            loader(filename, workspacename)
        except Exception, e:
            self.log().error("Failed to load run %s" % str(run))     

        # 3. Load title
        gfile = open(filename, "r")
        title = gfile.readline()
        gfile.close()

        return title


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
        instruments=["POWGEN", "NOMAD", "VULCAN"]
        self.declareProperty("Instrument", "POWGEN", Validator=ListValidator(instruments),
                Description="Powder diffractometer's name")
        self.declareListProperty("RunNumbers",[0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareFileProperty("OutputFile","", FileAction.Save, ['.gsa'],
                Description="The file conjoining all input GSAS files")
        self.declareProperty("Workspace", True, 
                Description="GSAS file will be made from existing Workspaces")
        self.declareFileProperty("Directory", "", FileAction.OptionalDirectory)
        self.declareProperty("ForPDFgetN", True,
                Description="Output file is for PDFgetN")
        self.declareProperty("MonitorValue", 2.0,
                Description="Monitor value for PDFgetN.  Default = 2.0")

        return


    def PyExec(self):
        """ Main exec
        """
        # 1. Get property
        instrument = self.getProperty("Instrument")
        runs = self.getProperty("RunNumbers")
        useworkspace = self.getProperty("Workspace")
        forpdfgetn = self.getProperty("ForPDFgetN")
        outputfilename = self.getPropertyValue("OutputFile")
        directory = self.getPropertyValue("Directory").strip()
        monitorvalue = self.getProperty("MonitorValue")

        instrumentheader = ""
        if instrument == "NOMAD":
            instrumentheader = "NOM"
        elif instrument == "POWGEN":
            instrumentheader = "PG3"
        elif instrument == "VULCAN":
            instrumentheader = "VUL"
        else:
            raise NotImplementedError("Impossible to have instrument name not defined")

        # 2. Load GSAS file if necessary
        joinedtitle = ""
        if useworkspace is not True:
            ext = ".gsa"
            for irun in xrange(len(runs)):
                # a) File name 
                filename = "%s_%s%s" % (instrumentheader, str(runs[irun]), ext)
                if len(directory) > 0:
                    filename = os.path.join(directory, filename)
                # b) Workspace
                wksp = "%s_%s" % (instrumentheader, str(runs[irun]))
                # c) Load
                loader = LoadGSS
                title = self.__loadGSASFile(filename, wksp, loader)
                if len(joinedtitle) == 0:
                    joinedtitle = title
            # ENDFOR
        # ENDIF

        # 2. Get Workspaces
        dataworkspaces = []
        # Using workspaces
        for irun in xrange(len(runs)):
            wsname = instrumentheader + "_" + str(runs[irun])
            ws = mtd[wsname]
            if ws is None:
                raise NotImplementedError("Workspace %s for instrument %s run %d is not in data service" % (wsname, instrument, runs[irun]))
            dataworkspaces.append(ws)
        # ENDFOR

        # 3. Write out
        print "Write Workspace %s" % (str(dataworkspaces[0]))
        SaveGSS(InputWorkspace=dataworkspaces[0], FileName=outputfilename, SplitFiles="False",
                Append=False, Bank=1, Format="SLOG", MultiplyByBinWidth=False)

        for iw in xrange(1, len(dataworkspaces)):
            print "Write workspace %s" % (str(dataworkspaces[iw]))
            SaveGSS(InputWorkspace=dataworkspaces[iw], FileName=outputfilename, SplitFiles="False",
                    Append=True, Bank=iw+1, Format="SLOG", MultiplyByBinWidth=False)
        # ENDFOR

        # 4. PDFgetN special
        if forpdfgetn is True:
            # Add monitor
            ofile = open(outputfilename, "r")
            lines = ofile.readlines()
            ofile.close()

            monitorterm = "Monitor: %f" % (monitorvalue)
            monitorline = "%-80s\n" % (monitorterm)
            lines.insert(1, monitorline)

            # Title
            if len(joinedtitle) > 0:
                lines.pop(0)
                lines.insert(0, joinedtitle)

            wbuf = ""
            for line in lines:
                wbuf += line

            ofile = open(outputfilename, "w")
            ofile.write(wbuf)
            ofile.close()


        # self._writeConjoinedGSASFile(filedicts, directory, outputfilename)
        # wksp = self.getPropertyValue("OutputWorkspace")
        # instr = mtd.getSettings().facility().instrument().shortName()

        return

mtd.registerPyAlgorithm(ConjoinGSASFiles())
