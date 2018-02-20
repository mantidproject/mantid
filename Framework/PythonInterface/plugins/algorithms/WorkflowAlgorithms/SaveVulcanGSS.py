#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from six.moves import range  #pylint: disable=redefined-builtin
import mantid.simpleapi as api
# from mantid.api import *
from mantid.kernel import *
from mantid.api import MatrixWorkspaceProperty, PropertyMode

class SaveVulcanGSS(PythonAlgorithm):
    """ Save GSS file for VULCAN.  This is a workflow algorithm
    """

    def category(self):
        """ category
        """
        return "Workflow\\Diffraction\\DataHandling"

    def name(self):
        """ name of algorithm
        """
        return "SaveVulcanGSS"

    def summary(self):
        """ Return summary
        """
        return "Save a focused EventWorkspace to GSAS file that is readable by VDRIVE"

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Focused diffraction workspace to be exported to GSAS file. ")

        self.declareProperty(TableWorkspaceProperty('BinningTable', '', Direction.Input, PropertyMode.Optional),
                             'Table workspace containing binning parameters. If not specified, then no re-binning'
                             'is required')

        # self.declareProperty(FileProperty("BinFilename", "", FileAction.OptionalLoad, ['.dat']),
        #                      "Name of a data file containing the bin boundaries in Log(TOF).")

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
        # Process input properties
        self._process_inputs()


        # rebin the input workspace if TOF binning file is given
        if len(log_tof_file_name) > 0:
            # Load reference bin file
            vec_refT = self._loadRefLogBinFile(log_tof_file_name)

            # Rebin
            gsa_ws = self._rebinVdrive(inputws, vec_refT, output_ws_name)
        else:
            # no binning file is specified. do nothing
            gsa_ws = inputws

        # Generate GSAS file
        output_ws = self._saveGSAS(gsa_ws, gss_file_name)

        # Convert header and bank information
        ipts = self.getPropertyValue("IPTS")
        parm_file_name = self.getPropertyValue("GSSParmFileName")
        new_header = self._generate_vulcan_gda_header(output_ws, gss_file_name, ipts, parm_file_name)
        self._rewrite_gda_file(gss_file_name, new_header)

        # Set property
        self.setProperty("OutputWorkspace", output_ws)

        return

    def _process_binning_parameters(self, input_workspace, bin_par_ws_name):
        """

        :param bin_par_ws_name:
        :return:
        """
        def convert_str_to_integers(list_str):
            """

            :param list_str:
            :return:
            """
            return list()

        bin_par_workspace = AnalysisDataService.retrieve(bin_par_ws_name)

        # check whether it is valid TableWorkspace
        if bin_par_workspace.columnCount() < 2:
            raise RuntimeError('Binning parameter table must have equal or more than 2 columns')

        # get workspace index
        binning_parameter_list = [None] * input_workspace.getNumberHistograms()

        # columns
        if bin_par_workspace.rowCount() == 1:
            # 1 binning parameter: uniform binning
            bin_par_str = bin_par_workspace.cell(0, 1)
            bin_parameters = self._process_binning_parameters(bin_par_str)
            binning_parameter_list = [bin_parameters] * input_workspace.getNumberHistograms()

        else:
            # each spectrum can have individual binning parameters
            binning_parameter_list = [None] * input_workspace.getNumberHistograms()

            for i_row in range(bin_par_workspace.rowCount()):
                ws_list_str = bin_par_workspace.cell(i_row, 0)
                ws_list = convert_str_to_integers(ws_list_str)

                # TODO FIXME NOW NOW2 Resume from here

        # END-IF-ELSE

        # check whether there is any spectrum that does not have been listed
        if None in binning_parameter_list:
            raise RuntimeError('Not all the spectra that have binning parameters set.')

        return binning_parameter_list

    def _process_inputs(self):
        """
        process input properties
        :return: input event workspace, binning parameter workspace, gsas file name, output workspace name
        """
        # get input properties
        input_workspace = self.getProperty("InputWorkspace")
        bin_par_ws_name = self.getPropertyValue('BinningTable')
        if len(bin_par_ws_name) > 0:
            bin_par_ws_exist = AnalysisDataService.doesExist(bin_par_ws_name)
        else:
            bin_par_ws_exist = False

        # event workspace is required for re-binning
        if input_workspace.id() != 'EventWorkspace' and bin_par_ws_exist:
            raise RuntimeError('Input workspace {0} must be an EventWorkspace if rebin is required by {1}'
                               ''.format(input_workspace, bin_par_ws_name))
        elif input_workspace.getAxis(0).getUnit().unitID() != "TOF":
            raise NotImplementedError("InputWorkspace must be in unit as TOF.")

        # processing binning parameters
        if bin_par_ws_exist:
            binning_parameter_list = self._process_binning_parameters(bin_par_ws_name)
        else:
            binning_parameter_list = None


        bin_par_workspace = self.getProperty('BinningTable')
        # log_tof_file_name = self.getPropertyValue("BinFilename")
        gss_file_name = self.getPropertyValue("GSSFilename")
        output_ws_name = self.getPropertyValue("OutputWorkspace")

        # Check properties
        inputws = AnalysisDataService.retrieve(input_workspace)
        if inputws is None:
            raise RuntimeError('Inputworkspace {0} does not exist.'.format(s))

        if inputws.isHistogramData() is False:
            raise NotImplementedError("InputWorkspace must be histogram, but not point data.")

        return input_workspace, bin_par_workspace, gss_file_name, output_ws_name

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
        dx = None
        for ibin in range(len(vec_refT)-1):
            x0 = vec_refT[ibin]
            xf = vec_refT[ibin+1]
            dx = xf-x0
            params.append(x0)
            params.append(dx)

        # last bin
        assert dx is not None, 'Vector of refT has less than 2 values.  It is not supported.'
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

    def _rewrite_gda_file(self, gssfilename, newheader):
        """
        Re-write GSAS file including header and header for each bank
        :param gssfilename:
        :param newheader:
        :return:
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

    def _generate_vulcan_gda_header(self, ws, gssfilename, ipts, parmfname):
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
                total_nanosecond_start = total_seconds * int(1.0E9) + int(runstart_ns)
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
        newheader += "%-80s\n" % title

        newheader += "%-80s\n" % ("Instrument parameter file: %s" % parmfname)

        newheader += "%-80s\n" % ("#IPTS: %s" % str(ipts))

        newheader += "%-80s\n" % "#binned by: Mantid"

        newheader += "%-80s\n" % ("#GSAS file name: %s" % os.path.basename(gssfilename))

        newheader += "%-80s\n" % ("#GSAS IPARM file: %s" % parmfname)

        newheader += "%-80s\n" % ("#Pulsestart:    %d" % total_nanosecond_start)

        newheader += "%-80s\n" % ("#Pulsestop:     %d" % total_nanosecond_stop)

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
