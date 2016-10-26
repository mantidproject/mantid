#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from six.moves import range #pylint: disable=redefined-builtin
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *


class SaveVulcanGSS(PythonAlgorithm):
    """ Save GSS file for VULCAN
    """

    def category(self):
        """
        """
        return "Diffraction\\DataHandling"

    def name(self):
        """
        """
        return "SaveVulcanGSS"

    def summary(self):
        """ Return summary
        """
        return "Load file generated by Fullprof."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Focussed diffraction workspace to be exported to GSAS file. ")

        self.declareProperty(FileProperty("BinFilename","", FileAction.Load, ['.dat']),
                             "Name of a data file containing the bin boundaries in Log(TOF). ")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             "Name of rebinned matrix workspace. ")

        self.declareProperty(FileProperty("GSSFilename","", FileAction.Save, ['.gda']),
                             "Name of the output GSAS file. ")

        self.declareProperty("IPTS", 0, "IPTS number")

        self.declareProperty("GSSParmFileName", "", "GSAS parameter file name for this GSAS data file.")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Properties
        inputwsname = self.getPropertyValue("InputWorkspace")
        logtoffilename = self.getPropertyValue("BinFilename")
        outgssfilename = self.getPropertyValue("GSSFilename")
        outputwsname = self.getPropertyValue("OutputWorkspace")

        # Check properties
        inputws = AnalysisDataService.retrieve(inputwsname)
        if inputws is None:
            raise NotImplementedError("Inputworkspace does not exist.")
        if inputws.getAxis(0).getUnit().unitID() != "TOF":
            raise NotImplementedError("InputWorkspace must be in unit as TOF.")
        if inputws.isHistogramData() is False:
            raise NotImplementedError("InputWorkspace must be histogram, but not point data.")

        # Load reference bin file
        vec_refT = self._loadRefLogBinFile(logtoffilename)

        # Rebin
        gsaws = self._rebinVdrive(inputws, vec_refT, outputwsname)

        # Generate GSAS file
        outputws = self._saveGSAS(gsaws, outgssfilename)

        # Convert header and bank information
        ipts = self.getPropertyValue("IPTS")
        parmfname = self.getPropertyValue("GSSParmFileName")
        newheader = self._genVulcanGSSHeader(outputws, outgssfilename, ipts, parmfname)
        self._rewriteGSSFile(outgssfilename, newheader)

        # Set property
        self.setProperty("OutputWorkspace", outputws)

        return

    def _loadRefLogBinFile(self, logbinfilename):
        """ Create a vector of bin in TOF value
        Arguments:
         - logbinfilename : name of file containing log_10(TOF) bins
        """
        import math

        bfile = open(logbinfilename, "r")
        lines = bfile.readlines()
        bfile.close()

        vecX = []
        for line in lines:
            line = line.strip()
            if len(line) == 0:
                continue
            if line[0] == "#":
                continue

            terms = line.split()
            for it in range(len(terms)):
                x = float(terms[it])
                vecX.append(x)
            # ENDFOR
        # ENDFOR

        vecPow10X = []
        for i in range(len(vecX)):
            p10x = math.pow(10, vecX[i])
            vecPow10X.append(p10x)

        return vecPow10X

    def _rebinVdrive(self, inputws, vec_refT, outputwsname):
        """ Rebin to match VULCAN's VDRIVE-generated GSAS file
        Arguments:
         - inputws : focussed workspace
         - vec_refT: list of TOF bins
        """
        # Create a complicated bin parameter
        params = []
        for ibin in range(len(vec_refT)-1):
            x0 = vec_refT[ibin]
            xf = vec_refT[ibin+1]
            dx = xf-x0
            params.append(x0)
            params.append(dx)

        # last bin
        x0 = vec_refT[-1]
        xf = 2*dx + x0
        params.extend([x0, 2*dx, xf])

        # Rebin
        tempws = api.Rebin(InputWorkspace=inputws, Params=params, PreserveEvents=False)

        # Map to a new workspace with 'vdrive-bin', which is the integer value of log bins
        numhist = tempws.getNumberHistograms()
        newvecx = []
        newvecy = []
        newvece = []
        for iws in range(numhist):
            vecx = tempws.readX(iws)
            vecy = tempws.readY(iws)
            vece = tempws.readE(iws)
            for i in range( len(vecx)-1 ):
                newvecx.append(int(vecx[i]*10)/10.)
                newvecy.append(vecy[i])
                newvece.append(vece[i])
            # ENDFOR (i)
        # ENDFOR (iws)
        api.DeleteWorkspace(Workspace=tempws)
        gsaws = api.CreateWorkspace(DataX=newvecx, DataY=newvecy, DataE=newvece, NSpec=numhist,
                                    UnitX="TOF", ParentWorkspace=inputws, OutputWorkspace=outputwsname)

        return gsaws

    def _saveGSAS(self, gsaws, gdafilename):
        """ Save file
        """
        # Convert from PointData to Histogram
        gsaws = api.ConvertToHistogram(InputWorkspace=gsaws, OutputWorkspace=str(gsaws))

        # Save
        api.SaveGSS(InputWorkspace=gsaws, Filename=gdafilename, SplitFiles=False, Append=False,
                    Format="SLOG", MultiplyByBinWidth=False, ExtendedHeader=False, UseSpectrumNumberAsBankID=True)

        return gsaws

    def _rewriteGSSFile(self, gssfilename, newheader):
        """ Re-write GSAS file including header and header for each bank
        """
        # Get all lines
        gfile = open(gssfilename, "r")
        lines = gfile.readlines()
        gfile.close()

        # New file
        filebuffer = ""
        filebuffer += newheader

        inbank = False
        banklines = []
        for line in lines:
            cline = line.strip()
            if len(cline) == 0:
                continue

            if line.startswith("BANK"):
                # Indicate a new bank
                if len(banklines) == 0:
                    # bank line for first bank
                    inbank = True
                    banklines.append(line.strip("\n"))
                else:
                    # bank line for non-first bank.
                    tmpbuffer = self._rewriteOneBankData(banklines)
                    filebuffer += tmpbuffer
                    banklines = [line]
                # ENDIFELSE
            elif inbank is True and cline.startswith("#") is False:
                # Write data line
                banklines.append(line.strip("\n"))

        # ENDFOR

        if len(banklines) > 0:
            tmpbuffer = self._rewriteOneBankData(banklines)
            filebuffer += tmpbuffer
        else:
            raise NotImplementedError("Impossible to have this")

        # Overwrite the original file
        ofile = open(gssfilename, "w")
        ofile.write(filebuffer)
        ofile.close()

        return

    def _genVulcanGSSHeader(self, ws, gssfilename, ipts, parmfname):
        """
        """
        from datetime import datetime
        import os.path

        # Get necessary information
        title = ws.getTitle()

        run = ws.getRun()

        # Get information on start/stop
        processtime = True
        if run.hasProperty("run_start") and run.hasProperty("duration"):
            runstart = run.getProperty("run_start").value
            duration = float(run.getProperty("duration").value)
        else:
            processtime = False

        if processtime is True:
            # property run_start and duration exist
            runstart_sec = runstart.split(".")[0]
            runstart_ns = runstart.split(".")[1]

            utctime = datetime.strptime(runstart_sec, '%Y-%m-%dT%H:%M:%S')
            time0=datetime.strptime("1990-01-01T0:0:0",'%Y-%m-%dT%H:%M:%S')

            delta = utctime-time0
            try:
                total_nanosecond_start =  int(delta.total_seconds()*int(1.0E9)) + int(runstart_ns)
            except AttributeError:
                total_seconds = delta.days*24*3600 + delta.seconds
                total_nanosecond_start = total_seconds * int(1.0E9)  + int(runstart_ns)
            total_nanosecond_stop = total_nanosecond_start + int(duration*1.0E9)
        else:
            # not both property is found
            total_nanosecond_start = 0
            total_nanosecond_stop = 0

        self.log().debug("Start = %d, Stop = %d" % (total_nanosecond_start, total_nanosecond_stop))

        # Construct new header
        newheader = ""

        if len(title) > 80:
            title = title[0:80]
        newheader += "%-80s\n" % (title)

        newheader += "%-80s\n" % ( "Instrument parameter file: %s" %(parmfname) )

        newheader += "%-80s\n" % ( "#IPTS: %s" % (str(ipts)) )

        newheader += "%-80s\n" % ( "#binned by: Mantid" )

        newheader += "%-80s\n" % ( "#GSAS file name: %s" % (os.path.basename(gssfilename)) )

        newheader += "%-80s\n" % ( "#GSAS IPARM file: %s" % (parmfname) )

        newheader += "%-80s\n" % ( "#Pulsestart:    %d" % (total_nanosecond_start) )

        newheader += "%-80s\n" % ( "#Pulsestop:     %d" % (total_nanosecond_stop) )

        return newheader

    def _rewriteOneBankData(self, banklines):
        """ first line is for bank information
        """
        wbuf = ""

        # Rewrite bank lines
        bankline = banklines[0].strip()
        terms = bankline.split()
        tofmin = float(banklines[1].split()[0])
        tofmax = float(banklines[-1].split()[0])

        terms[5] = "%.1f" % (tofmin)
        terms[6] = "%.1f" % (tofmax)

        newbankline = ""

        # title
        for t in terms:
            newbankline += "%s " % (t)
        wbuf = "%-80s\n" % (newbankline)

        # data
        for i in range(1, len(banklines)):
            cline = banklines[i]

            terms = cline.split()
            try:
                tof = float(terms[0])
                y = float(terms[1])
                e = float(terms[2])

                x_s = "%.1f" % (tof)
                y_s = "%.1f" % (y)
                e_s = "%.2f" % (e)

                temp = "%12s%12s%12s" % (x_s, y_s, e_s)

            except TypeError:
                temp = "%-80s\n" % (cline.rstrip())

            wbuf += "%-80s\n" % (temp)
        # ENDFOR

        return wbuf


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SaveVulcanGSS)
