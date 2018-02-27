#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from six.moves import range  #pylint: disable=redefined-builtin
import mantid.simpleapi as api
# from mantid.api import *
# from mantid.kernel import *
from mantid.api import AnalysisDataService
from mantid.api import MatrixWorkspaceProperty, PropertyMode, PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty
from mantid.api import FileProperty, FileAction, MatrixWorkspace, ITableWorkspace
from mantid.kernel import Direction
import numpy


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

        self.declareProperty(ITableWorkspaceProperty('BinningTable', '', Direction.Input, PropertyMode.Optional),
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
        input_ws, binning_param_list, gsas_file_name, output_ws_name = self.process_inputs()

        # Rebin workspace
        output_workspace = self.rebin_workspace(input_ws, binning_param_list, output_ws_name)

        # #
        #
        # # rebin the input workspace if TOF binning file is given
        # if len(log_tof_file_name) > 0:
        #     # Load reference bin file
        #     vec_refT = self._loadRefLogBinFile(log_tof_file_name)
        #
        #     # Rebin
        #     gsa_ws = self._rebinVdrive(inputws, vec_refT, output_ws_name)
        # else:
        #     # no binning file is specified. do nothing
        #     gsa_ws = inputws
        #
        # # Generate GSAS file
        # output_ws = self._saveGSAS(gsa_ws, gss_file_name)
        #
        # # Convert header and bank information
        # ipts = self.getPropertyValue("IPTS")
        # parm_file_name = self.getPropertyValue("GSSParmFileName")
        # new_header = self._generate_vulcan_gda_header(output_ws, gss_file_name, ipts, parm_file_name)
        # self._rewrite_gda_file(gss_file_name, new_header)

        # Set property
        self.setProperty("OutputWorkspace", output_workspace)

        return

    @staticmethod
    def rebin_workspace(input_ws, binning_param_list, output_ws_name):
        """
        rebin input workspace with user specified binning parameters
        :param input_ws:
        :param binning_param_list:
        :param output_ws_name:
        :return:
        """
        if binning_param_list is None:
            # no re-binning is required: clone the output workspace
            output_workspace = api.CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=output_ws_name)

        else:
            # rebin input workspace
            processed_single_spec_ws_list = list()
            for ws_index in range(input_ws.getNumberHistograms()):
                # rebin on each
                temp_out_name = output_ws_name + '_' + str(ws_index)
                processed_single_spec_ws_list.append(temp_out_name)
                # extract a spectrum out
                api.ExtractSpectra(input_ws, WorkspaceIndexList=[ws_index], OutputWorkspace=temp_out_name)
                # get binning parameter
                bin_params = binning_param_list[ws_index]
                if bin_params is None:
                    continue
                # rebin
                # check
                if len(bin_params) % 2 == 0:
                    # odd number and cannot be binning parameters
                    raise RuntimeError('Binning parameter {0} cannot be accepted.'.format(bin_params))

                api.Rebin(InputWorkspace=temp_out_name, OutputWorkspace=temp_out_name,
                          Params=bin_params, PreserveEvents=True)
                # convert to point data
                api.ConvertToPointData(InputWorkspace=temp_out_name, OutputWorkspace=temp_out_name)
                # align the bin boundaries if necessary
                temp_out_ws = AnalysisDataService.retrieve(temp_out_name)
                if len(bin_params) == len(temp_out_ws.readX(0)):
                    # good to align
                    for tof_i in range(len(bin_params)):
                        temp_out_ws.dataX[tof_i] = int(bin_params[tof_i] * 10) / 10.
                    # END-FOR (tof-i)
                # END-IF (align)
            # END-FOR

            # merge together
            api.RenameWorkspace(InputWorkspace=processed_single_spec_ws_list[0],
                                OutputWorkspace=output_ws_name)
            for ws_index in range(1, len(processed_single_spec_ws_list)):
                api.ConjoinWorkspaces(InputWorkspace1=output_ws_name,
                                      InputWorkspace2=processed_single_spec_ws_list[ws_index])
            # END-FOR
            output_workspace = AnalysisDataService.retrieve(output_ws_name)
        # END-IF-ELSE

        return output_workspace

    def process_inputs(self):
        """
        process input properties
        :return: input event workspace, binning parameter workspace, gsas file name, output workspace name
        """
        # get input properties
        input_ws_name = self.getPropertyValue("InputWorkspace")
        bin_par_ws_name = self.getPropertyValue('BinningTable')
        if len(bin_par_ws_name) > 0:
            bin_par_ws_exist = AnalysisDataService.doesExist(bin_par_ws_name)
        else:
            bin_par_ws_exist = False

        # event workspace is required for re-binning
        input_workspace = AnalysisDataService.retrieve(input_ws_name)
        if input_workspace.id() != 'EventWorkspace' and bin_par_ws_exist:
            self.log().warning('Input workspace {0} must be an EventWorkspace if rebin is required by {1}'
                               ''.format(input_workspace, bin_par_ws_name))
        elif input_workspace.getAxis(0).getUnit().unitID() != "TOF":
            raise NotImplementedError("InputWorkspace must be in unit as TOF.")

        # processing binning parameters
        if bin_par_ws_exist:
            binning_parameter_list = self.process_binning_param_table(input_workspace, bin_par_ws_name)
        else:
            binning_parameter_list = None

        # gsas file name (output)
        gss_file_name = self.getPropertyValue("GSSFilename")

        # output workspace name
        output_ws_name = self.getPropertyValue("OutputWorkspace")

        return input_workspace, binning_parameter_list, gss_file_name, output_ws_name

    @staticmethod
    def generate_bin_parameters_from_workspace(ref_tof_ws, ws_index):
        """
        generate a vector as binning parameters from reference TOF binning workspace
        :param ref_tof_ws:
        :param ws_index:
        :return:
        """
        # Check input
        assert isinstance(ref_tof_ws, MatrixWorkspace), 'TOF reference workspace must be a MatrixWorkspace but ' \
                                                        'not a {0}'.format(ref_tof_ws.__class__.__name__)
        assert isinstance(ws_index, int), 'Workspace index {0} must be an integer but not a {1}' \
                                          ''.format(ws_index, type(ws_index))

        # Create a complicated bin parameter
        if ws_index < 0 or ws_index >= ref_tof_ws.getNumberOfHistograms():
            raise RuntimeError('Workspace {0} is out of workspace indexes range [0, {1})'
                               ''.format(ws_index, ref_tof_ws.getNumberOfHistograms()))
        else:
            # get the TOF vector
            vec_ref_tof = ref_tof_ws.readX(ws_index)

        params = list()
        dx = None
        for ibin in range(len(vec_ref_tof) - 1):
            x0 = vec_ref_tof[ibin]
            xf = vec_ref_tof[ibin + 1]
            dx = xf - x0
            params.append(x0)
            params.append(dx)

        # last bin: last bin is not defined by input TOF reference workspace. Set a reasonably one with 2*dX
        assert dx is not None, 'Vector of refT has less than 2 values.  It is not supported.'
        x0 = vec_ref_tof[-1]
        xf = 2 * dx + x0  # last bin is out of defined range
        params.extend([x0, 2 * dx, xf])

        # Rebin ...
        # tempws = api.Rebin(InputWorkspace=input_ws, Params=params, PreserveEvents=True)

        return params

    def process_binning_param_table(self, input_workspace, bin_par_ws_name):
        """
        process the binning parameters given in an ITableWorkspace
        :param input_workspace:
        :param bin_par_ws_name:
        :return:
        """
        def convert_str_to_integers(list_str):
            """
            convert a string to positive integers.  it could be a-b, c, d
            :param list_str:
            :return:
            """
            terms = list_str.replace(' ', '').split(',')
            int_list = list()
            for term in terms:
                term = term.strip()
                if term.count('-') == 0:
                    # case of single integer
                    try:
                        int_value = int(term)
                    except ValueError:
                        raise RuntimeError('{0} cannot be converted to integer'.format(term))
                    int_list.append(int_value)
                elif term.count('-') == 1:
                    # case of integer range
                    two_parts = term.split('-')
                    if len(two_parts) != 2:
                        raise RuntimeError('{0} cannot be a negative number and must be in format X-Y'.format(term))
                    try:
                        i0 = int(two_parts[0])
                        i1 = int(two_parts[1])
                    except ValueError:
                        raise RuntimeError('{0} cannot be converted to integers.'.format(two_parts))
                    int_list.extend(range(i0, i1+1))
                else:
                    raise RuntimeError('{0} cannot have any negative number and must be in format X-Y'.format(term))
            # END-FOR (term)

            return int_list
        # END-DEF

        bin_par_workspace = AnalysisDataService.retrieve(bin_par_ws_name)
        # check inputs
        assert isinstance(bin_par_workspace, ITableWorkspace), 'Input binning workspace {0} must be ' \
                                                               'an ITableWorkspace but not a {1}' \
                                                               ''.format(bin_par_workspace,
                                                                         type(bin_par_workspace))

        # check whether it is valid TableWorkspace
        if bin_par_workspace.columnCount() < 2:
            raise RuntimeError('Binning parameter table must have equal or more than 2 columns')

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
                # get workspace indexes
                ws_list_str = bin_par_workspace.cell(i_row, 0)
                ws_list = convert_str_to_integers(ws_list_str)

                # process the binning parameters
                bin_par_str = bin_par_workspace.cell(i_row, 1)
                bin_parameters = self._process_binning_parameters(bin_par_str)
                for ws_index in ws_list:
                    binning_parameter_list[ws_index] = bin_parameters
                # END-FOR
            # END-FOR (i_row)
        # END-IF-ELSE

        # check whether there is any spectrum that does not have been listed
        if None in binning_parameter_list:
            raise RuntimeError('Not all the spectra that have binning parameters set.')

        return binning_parameter_list

    @staticmethod
    def _process_binning_parameters(bin_par_str):
        """
        process binning parameters in string.  there are two types of binning parameters that are accepted
        1. regular x0, dx0, x1, dx1, etc or
        2. workspace name: workspace index
        :param bin_par_str:
        :return:
        """
        if bin_par_str.count(':') == 0:
            # parse regular binning parameters
            terms = bin_par_str.split(',')  # in string format
            bin_param = list()
            try:
                for term in terms:
                    bin_param.append(float(term))
            except ValueError:
                raise RuntimeError('Binning parameters {0} have non-float terms.'.format(bin_par_str))

        elif bin_par_str.count(':') == 1:
            # in workspace name : workspace index mode
            terms = bin_par_str.split(':')
            ref_ws_name = terms[0].strip()
            if AnalysisDataService.doesExist(ref_ws_name) is False:
                raise RuntimeError('Workspace {0} does not exist (FYI {1})'
                                   ''.format(ref_ws_name, bin_par_str))
            try:
                ws_index = int(terms[1].strip())
            except ValueError:
                raise RuntimeError('blabla')

            ref_tof_ws = AnalysisDataService.retrieve(ref_ws_name)
            if ws_index < 0 or ws_index >= ref_tof_ws.getNumberHistograms():
                raise RuntimeError('blabla2')

            ref_tof_vec = ref_tof_ws.readX(ws_index)
            delta_tof_vec = ref_tof_vec[1:] - ref_tof_vec[:-1]

            bin_param = numpy.empty((ref_tof_vec.size + delta_tof_vec.size), dtype=ref_tof_vec.dtype)
            bin_param[0::2] = ref_tof_vec
            bin_param[1::2] = delta_tof_vec

        else:
            raise RuntimeError('Not .... blabla3')

        return bin_param

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
