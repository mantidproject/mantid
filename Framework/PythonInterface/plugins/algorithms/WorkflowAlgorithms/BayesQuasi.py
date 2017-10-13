# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init,redefined-builtin
from __future__ import (absolute_import, division, print_function)
from six.moves import range
from six import next

import os
import numpy as np

from IndirectImport import *
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode,
                        WorkspaceGroupProperty, Progress)
from mantid.kernel import StringListValidator, Direction
import mantid.simpleapi as s_api
from mantid import config, logger
from IndirectCommon import *

MTD_PLOT = import_mantidplot()

if is_supported_f2py_platform():
    QLr = import_f2py("QLres")
    QLd = import_f2py("QLdata")
    Qse = import_f2py("QLse")


class BayesQuasi(PythonAlgorithm):
    _program = None
    _samWS = None
    _resWS = None
    _resnormWS = None
    _e_min = None
    _e_max = None
    _sam_bins = None
    _res_bins = None
    _elastic = None
    _background = None
    _width = None
    _res_norm = None
    _wfile = None
    _loop = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "This algorithm runs the Fortran QLines programs which fits a Delta function of" + \
               " amplitude 0 and Lorentzians of amplitude A(j) and HWHM W(j) where j=1,2,3. The" + \
               " whole function is then convolved with the resolution function."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(name='Program', defaultValue='QL',
                             validator=StringListValidator(['QL', 'QSe']),
                             doc='The type of program to run (either QL or QSe)')

        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Name of the Sample input Workspace')

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '', direction=Direction.Input),
                             doc='Name of the resolution input Workspace')

        self.declareProperty(WorkspaceGroupProperty('ResNormWorkspace', '',
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Input),
                             doc='Name of the ResNorm input Workspace')

        self.declareProperty(name='MinRange', defaultValue=-0.2,
                             doc='The start of the fit range. Default=-0.2')

        self.declareProperty(name='MaxRange', defaultValue=0.2,
                             doc='The end of the fit range. Default=0.2')

        self.declareProperty(name='SampleBins', defaultValue=1,
                             doc='The number of sample bins')

        self.declareProperty(name='ResolutionBins', defaultValue=1,
                             doc='The number of resolution bins')

        self.declareProperty(name='Elastic', defaultValue=True,
                             doc='Fit option for using the elastic peak')

        self.declareProperty(name='Background', defaultValue='Flat',
                             validator=StringListValidator(['Sloping', 'Flat', 'Zero']),
                             doc='Fit option for the type of background')

        self.declareProperty(name='FixedWidth', defaultValue=True,
                             doc='Fit option for using FixedWidth')

        self.declareProperty(name='UseResNorm', defaultValue=False,
                             doc='fit option for using ResNorm')

        self.declareProperty(name='WidthFile', defaultValue='', doc='The name of the fixedWidth file')

        self.declareProperty(name='Loop', defaultValue=True, doc='Switch Sequential fit On/Off')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceFit', '', direction=Direction.Output),
                             doc='The name of the fit output workspaces')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceResult', '', direction=Direction.Output),
                             doc='The name of the result output workspaces')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceProb', '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The name of the probability output workspaces')

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate fitting range in energy
        if self._e_min > self._e_max:
            issues['MaxRange'] = 'Must be less than EnergyMin'

        return issues

    def _get_properties(self):
        self._program = self.getPropertyValue('Program')
        self._samWS = self.getPropertyValue('SampleWorkspace')
        self._resWS = self.getPropertyValue('ResolutionWorkspace')
        self._resnormWS = self.getPropertyValue('ResNormWorkspace')
        self._e_min = self.getProperty('MinRange').value
        self._e_max = self.getProperty('MaxRange').value
        self._sam_bins = self.getPropertyValue('SampleBins')
        self._res_bins = self.getPropertyValue('ResolutionBins')
        self._elastic = self.getProperty('Elastic').value
        self._background = self.getPropertyValue('Background')
        self._width = self.getProperty('FixedWidth').value
        self._res_norm = self.getProperty('UseResNorm').value
        self._wfile = self.getPropertyValue('WidthFile')
        self._loop = self.getProperty('Loop').value

    # pylint: disable=too-many-locals,too-many-statements
    def PyExec(self):

        self.check_platform_support()

        from IndirectBayes import (CalcErange, GetXYE)
        setup_prog = Progress(self, start=0.0, end=0.3, nreports=5)
        self.log().information('BayesQuasi input')

        erange = [self._e_min, self._e_max]
        nbins = [self._sam_bins, self._res_bins]
        setup_prog.report('Converting to binary for Fortran')
        # convert true/false to 1/0 for fortran
        o_el = int(self._elastic)
        o_w1 = int(self._width)
        o_res = int(self._res_norm)

        # fortran code uses background choices defined using the following numbers
        setup_prog.report('Encoding input options')
        o_bgd = ['Zero', 'Flat', 'Sloping'].index(self._background)
        fitOp = [o_el, o_bgd, o_w1, o_res]

        setup_prog.report('Establishing save path')
        workdir = config['defaultsave.directory']
        if not os.path.isdir(workdir):
            workdir = os.getcwd()
            logger.information('Default Save directory is not set. Defaulting to current working Directory: ' + workdir)

        array_len = 4096  # length of array in Fortran
        setup_prog.report('Checking X Range')
        CheckXrange(erange, 'Energy')

        nbin, nrbin = nbins[0], nbins[1]

        logger.information('Sample is ' + self._samWS)
        logger.information('Resolution is ' + self._resWS)

        # Check for trailing and leading zeros in data
        setup_prog.report('Checking for leading and trailing zeros in the data')
        first_data_point, last_data_point = IndentifyDataBoundaries(self._samWS)
        self.check_energy_range_for_zeroes(first_data_point, last_data_point)

        # update erange with new values
        erange = [self._e_min, self._e_max]

        setup_prog.report('Checking Analysers')
        CheckAnalysers(self._samWS, self._resWS)
        setup_prog.report('Obtaining EFixed, theta and Q')
        efix = getEfixed(self._samWS)
        theta, Q = GetThetaQ(self._samWS)

        nsam, ntc = CheckHistZero(self._samWS)

        totalNoSam = nsam

        # check if we're performing a sequential fit
        if not self._loop:
            nsam = 1

        nres = CheckHistZero(self._resWS)[0]

        setup_prog.report('Checking Histograms')
        if self._program == 'QL':
            if nres == 1:
                prog = 'QLr'  # res file
            else:
                prog = 'QLd'  # data file
                CheckHistSame(self._samWS, 'Sample', self._resWS, 'Resolution')
        elif self._program == 'QSe':
            if nres == 1:
                prog = 'QSe'  # res file
            else:
                raise ValueError('Stretched Exp ONLY works with RES file')

        logger.information('Version is ' + prog)
        logger.information(' Number of spectra = ' + str(nsam))
        logger.information(' Erange : ' + str(erange[0]) + ' to ' + str(erange[1]))

        setup_prog.report('Reading files')
        Wy, We = self._read_width_file(self._width, self._wfile, totalNoSam)
        dtn, xsc = self._read_norm_file(self._res_norm, self._resnormWS, totalNoSam)

        setup_prog.report('Establishing output workspace name')
        fname = self._samWS[:-4] + '_' + prog
        probWS = fname + '_Prob'
        fitWS = fname + '_Fit'
        wrks = os.path.join(workdir, self._samWS[:-4])
        logger.information(' lptfile : ' + wrks + '_' + prog + '.lpt')
        lwrk = len(wrks)
        wrks.ljust(140, ' ')
        wrkr = self._resWS
        wrkr.ljust(140, ' ')

        setup_prog.report('Initialising probability list')
        # initialise probability list
        if self._program == 'QL':
            prob0, prob1, prob2 = [], [], []
        xQ = np.array([Q[0]])
        for m in range(1, nsam):
            xQ = np.append(xQ, Q[m])
        xProb = xQ
        xProb = np.append(xProb, xQ)
        xProb = np.append(xProb, xQ)
        eProb = np.zeros(3 * nsam)

        group = ''
        workflow_prog = Progress(self, start=0.3, end=0.7, nreports=nsam * 3)
        for spectrum in range(0, nsam):
            logger.information('Group ' + str(spectrum) + ' at angle ' + str(theta[spectrum]))
            nsp = spectrum + 1

            nout, bnorm, Xdat, Xv, Yv, Ev = CalcErange(self._samWS, spectrum, erange, nbin)
            Ndat = nout[0]
            Imin = nout[1]
            Imax = nout[2]
            if prog == 'QLd':
                mm = spectrum
            else:
                mm = 0
            Nb, Xb, Yb, Eb = GetXYE(self._resWS, mm, array_len)  # get resolution data
            numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin]
            rscl = 1.0
            reals = [efix, theta[spectrum], rscl, bnorm]

            if prog == 'QLr':
                workflow_prog.report('Processing Sample number %i as Lorentzian' % spectrum)
                nd, xout, yout, eout, yfit, yprob = QLr.qlres(numb, Xv, Yv, Ev, reals, fitOp,
                                                              Xdat, Xb, Yb, Wy, We, dtn, xsc,
                                                              wrks, wrkr, lwrk)
                message = ' Log(prob) : ' + str(yprob[0]) + ' ' + str(yprob[1]) + ' ' + str(yprob[2]) + ' ' + str(
                    yprob[3])
                logger.information(message)
            elif prog == 'QLd':
                workflow_prog.report('Processing Sample number %i' % spectrum)
                nd, xout, yout, eout, yfit, yprob = QLd.qldata(numb, Xv, Yv, Ev, reals, fitOp,
                                                               Xdat, Xb, Yb, Eb, Wy, We,
                                                               wrks, wrkr, lwrk)
                message = ' Log(prob) : ' + str(yprob[0]) + ' ' + str(yprob[1]) + ' ' + str(yprob[2]) + ' ' + str(
                    yprob[3])
                logger.information(message)
            elif prog == 'QSe':
                workflow_prog.report('Processing Sample number %i as Stretched Exp' % spectrum)
                nd, xout, yout, eout, yfit, yprob = Qse.qlstexp(numb, Xv, Yv, Ev, reals, fitOp,
                                                                Xdat, Xb, Yb, Wy, We, dtn, xsc,
                                                                wrks, wrkr, lwrk)

            dataX = xout[:nd]
            dataX = np.append(dataX, 2 * xout[nd - 1] - xout[nd - 2])
            yfit_list = np.split(yfit[:4 * nd], 4)
            dataF1 = yfit_list[1]
            workflow_prog.report('Processing data')
            dataG = np.zeros(nd)
            datX = dataX
            datY = yout[:nd]
            datE = eout[:nd]
            datX = np.append(datX, dataX)
            datY = np.append(datY, dataF1[:nd])
            datE = np.append(datE, dataG)
            res1 = dataF1[:nd] - yout[:nd]
            datX = np.append(datX, dataX)
            datY = np.append(datY, res1)
            datE = np.append(datE, dataG)
            nsp = 3
            names = 'data,fit.1,diff.1'
            res_plot = [0, 1, 2]
            if self._program == 'QL':
                workflow_prog.report('Processing Lorentzian result data')
                dataF2 = yfit_list[2]
                datX = np.append(datX, dataX)
                datY = np.append(datY, dataF2[:nd])
                datE = np.append(datE, dataG)
                res2 = dataF2[:nd] - yout[:nd]
                datX = np.append(datX, dataX)
                datY = np.append(datY, res2)
                datE = np.append(datE, dataG)
                nsp += 2
                names += ',fit.2,diff.2'
                res_plot.append(4)
                prob0.append(yprob[0])
                prob1.append(yprob[1])
                prob2.append(yprob[2])

            # create result workspace
            fitWS = fname + '_Workspaces'
            fout = fname + '_Workspace_' + str(spectrum)

            workflow_prog.report('Creating OutputWorkspace')
            s_api.CreateWorkspace(OutputWorkspace=fout, DataX=datX, DataY=datY, DataE=datE,
                                  Nspec=nsp, UnitX='DeltaE', VerticalAxisUnit='Text', VerticalAxisValues=names)

            # append workspace to list of results
            group += fout + ','

        comp_prog = Progress(self, start=0.7, end=0.8, nreports=2)
        comp_prog.report('Creating Group Workspace')
        s_api.GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=fitWS)

        if self._program == 'QL':
            comp_prog.report('Processing Lorentzian probability data')
            yPr0 = np.array([prob0[0]])
            yPr1 = np.array([prob1[0]])
            yPr2 = np.array([prob2[0]])
            for m in range(1, nsam):
                yPr0 = np.append(yPr0, prob0[m])
                yPr1 = np.append(yPr1, prob1[m])
                yPr2 = np.append(yPr2, prob2[m])
            yProb = yPr0
            yProb = np.append(yProb, yPr1)
            yProb = np.append(yProb, yPr2)
            s_api.CreateWorkspace(OutputWorkspace=probWS, DataX=xProb, DataY=yProb, DataE=eProb,
                                  Nspec=3, UnitX='MomentumTransfer')
            outWS = self.C2Fw(fname)
        elif self._program == 'QSe':
            comp_prog.report('Running C2Se')
            outWS = self.C2Se(fname)

        # Sort x axis
        s_api.SortXAxis(InputWorkspace=outWS, OutputWorkspace=outWS, EnableLogging=False)

        log_prog = Progress(self, start=0.8, end=1.0, nreports=8)
        # Add some sample logs to the output workspaces
        log_prog.report('Copying Logs to outputWorkspace')
        s_api.CopyLogs(InputWorkspace=self._samWS, OutputWorkspace=outWS)
        log_prog.report('Adding Sample logs to Output workspace')
        self._add_sample_logs(outWS, prog, erange, nbins)
        log_prog.report('Copying logs to fit Workspace')
        s_api.CopyLogs(InputWorkspace=self._samWS, OutputWorkspace=fitWS)
        log_prog.report('Adding sample logs to Fit workspace')
        self._add_sample_logs(fitWS, prog, erange, nbins)
        log_prog.report('Finalising log copying')

        self.setProperty('OutputWorkspaceFit', fitWS)
        self.setProperty('OutputWorkspaceResult', outWS)
        log_prog.report('Setting workspace properties')

        if self._program == 'QL':
            s_api.SortXAxis(InputWorkspace=probWS, OutputWorkspace=probWS, EnableLogging=False)
            self.setProperty('OutputWorkspaceProb', probWS)

    def check_platform_support(self):
        if not is_supported_f2py_platform():
            unsupported_msg = "This algorithm can only be run on valid platforms." \
                              + " please view the algorithm documentation to see" \
                              + " what platforms are currently supported"
            raise RuntimeError(unsupported_msg)

    def check_energy_range_for_zeroes(self, first_data_point, last_data_point):
        if first_data_point > self._e_min:
            logger.warning("Sample workspace contains leading zeros within the energy range.")
            logger.warning("Updating eMin: eMin = " + str(first_data_point))
            self._e_min = first_data_point
        if last_data_point < self._e_max:
            logger.warning("Sample workspace contains trailing zeros within the energy range.")
            logger.warning("Updating eMax: eMax = " + str(last_data_point))
            self._e_max = last_data_point

    def _add_sample_logs(self, workspace, fit_program, e_range, binning):

        sample_binning, res_binning = binning
        energy_min, energy_max = e_range

        sample_logs = [('res_workspace', self._resWS),
                       ('fit_program', fit_program),
                       ('background', self._background),
                       ('elastic_peak', self._elastic),
                       ('energy_min', energy_min),
                       ('energy_max', energy_max),
                       ('sample_binning', sample_binning),
                       ('resolution_binning', res_binning)]

        resnorm_used = (self._resnormWS != '')
        sample_logs.append(('resnorm', str(resnorm_used)))
        if resnorm_used:
            sample_logs.append(('resnorm_file', str(self._resnormWS)))

        width_file_used = (self._wfile != '')
        sample_logs.append(('width', str(width_file_used)))
        if width_file_used:
            sample_logs.append(('width_file', str(self._wfile)))

        log_alg = self.createChildAlgorithm('AddSampleLogMultiple', 0.9, 1.0, False)
        log_alg.setProperty('Workspace', workspace)
        log_alg.setProperty('LogNames', [log[0] for log in sample_logs])
        log_alg.setProperty('LogValues', [log[1] for log in sample_logs])
        log_alg.execute()

    def C2Se(self, sname):
        outWS = sname + '_Result'
        asc = self._read_ascii_file(sname + '.qse')
        var = asc[3].split()  # split line on spaces
        nspec = var[0]
        var = ExtractInt(asc[6])
        first = 7
        Xout = []
        Yf, Yi, Yb = [], [], []
        Ef, Ei, Eb = [], [], []
        ns = int(nspec)

        dataX = np.array([])
        dataY = np.array([])
        dataE = np.array([])
        data = np.array([dataX, dataY, dataE])

        for _ in range(0, ns):
            first, Q, _, fw, it, be = self.SeBlock(asc, first)
            Xout.append(Q)
            Yf.append(fw[0])
            Ef.append(fw[1])
            Yi.append(it[0])
            Ei.append(it[1])
            Yb.append(be[0])
            Eb.append(be[1])
        Vaxis = []

        dataX, dataY, dataE, data = self._add_xye_data(data, Xout, Yi, Ei)
        nhist = 1
        Vaxis.append('f1.Amplitude')

        dataX, dataY, dataE, data = self._add_xye_data(data, Xout, Yf, Ef)
        nhist += 1
        Vaxis.append('f1.FWHM')

        dataX, dataY, dataE, data = self._add_xye_data(data, Xout, Yb, Eb)
        nhist += 1
        Vaxis.append('f1.Beta')

        logger.information('Vaxis=' + str(Vaxis))
        s_api.CreateWorkspace(OutputWorkspace=outWS, DataX=dataX, DataY=dataY, DataE=dataE, Nspec=nhist,
                              UnitX='MomentumTransfer', VerticalAxisUnit='Text', VerticalAxisValues=Vaxis,
                              YUnitLabel='')

        return outWS

    def _add_xye_data(self, data, xout, Y, E):

        dX, dY, dE = data[0], data[1], data[2]
        dX = np.append(dX, np.array(xout))
        dY = np.append(dY, np.array(Y))
        dE = np.append(dE, np.array(E))
        data = (dX, dY, dE)

        return dX, dY, dE, data

    def _read_ascii_file(self, file_name):
        workdir = config['defaultsave.directory']
        file_path = os.path.join(workdir, file_name)
        asc = []
        with open(file_path, 'U') as handle:
            for line in handle:
                line = line.rstrip()
                asc.append(line)
        return asc

    def SeBlock(self, a, index):  # read Ascii block of Integers
        index += 1
        val = ExtractFloat(a[index])  # Q,AMAX,HWHM
        Q = val[0]
        AMAX = val[1]
        HWHM = val[2]
        index += 1
        val = ExtractFloat(a[index])  # A0
        int0 = [AMAX * val[0]]
        index += 1
        val = ExtractFloat(a[index])  # AI,FWHM index peak
        fw = [2. * HWHM * val[1]]
        integer = [AMAX * val[0]]
        index += 1
        val = ExtractFloat(a[index])  # SIG0
        int0.append(val[0])
        index += 1
        val = ExtractFloat(a[index])  # SIG3K
        integer.append(AMAX * math.sqrt(math.fabs(val[0]) + 1.0e-20))
        index += 1
        val = ExtractFloat(a[index])  # SIG1K
        fw.append(2.0 * HWHM * math.sqrt(math.fabs(val[0]) + 1.0e-20))
        index += 1
        be = ExtractFloat(a[index])  # EXPBET
        index += 1
        val = ExtractFloat(a[index])  # SIG2K
        be.append(math.sqrt(math.fabs(val[0]) + 1.0e-20))
        index += 1
        return index, Q, int0, fw, integer, be  # values as list

    def _get_res_norm(self, resnormWS, ngrp):
        if ngrp == 0:  # read values from WS
            dtnorm = s_api.mtd[resnormWS + '_Intensity'].readY(0)
            xscale = s_api.mtd[resnormWS + '_Stretch'].readY(0)
        else:  # constant values
            dtnorm = []
            xscale = []
            for _ in range(0, ngrp):
                dtnorm.append(1.0)
                xscale.append(1.0)
        dtn = PadArray(dtnorm, 51)  # pad for Fortran call
        xsc = PadArray(xscale, 51)
        return dtn, xsc

    def _read_norm_file(self, readRes, resnormWS, nsam):  # get norm & scale values
        resnorm_root = resnormWS
        # Obtain root of resnorm group name
        if '_Intensity' in resnormWS:
            resnorm_root = resnormWS[:-10]
        if '_Stretch' in resnormWS:
            resnorm_root = resnormWS[:-8]

        if readRes:  # use ResNorm file option=o_res
            Xin = s_api.mtd[resnorm_root + '_Intensity'].readX(0)
            nrm = len(Xin)  # no. points from length of x array
            if nrm == 0:
                raise ValueError('ResNorm file has no Intensity points')
            Xin = s_api.mtd[resnorm_root + '_Stretch'].readX(0)  # no. points from length of x array
            if len(Xin) == 0:
                raise ValueError('ResNorm file has no xscale points')
            if nrm != nsam:  # check that no. groups are the same
                raise ValueError('ResNorm groups (' + str(nrm) + ') not = Sample (' + str(nsam) + ')')
            else:
                dtn, xsc = self._get_res_norm(resnorm_root, 0)
        else:
            # do not use ResNorm file
            dtn, xsc = self._get_res_norm(resnorm_root, nsam)
        return dtn, xsc

    # Reads in a width ASCII file
    def _read_width_file(self, readWidth, widthFile, numSampleGroups):
        widthY, widthE = [], []
        if readWidth:
            logger.information('Width file is ' + widthFile)
            # read ascii based width file
            try:
                wfPath = s_api.FileFinder.getFullPath(widthFile)
                handle = open(wfPath, 'r')
                asc = []
                for line in handle:
                    line = line.rstrip()
                    asc.append(line)
                handle.close()
            except Exception:
                raise ValueError('Failed to read width file')
            numLines = len(asc)
            if numLines == 0:
                raise ValueError('No groups in width file')
            if numLines != numSampleGroups:  # check that no. groups are the same
                raise ValueError('Width groups (' + str(numLines) + ') not = Sample (' + str(numSampleGroups) + ')')
        else:
            # no file: just use constant values
            widthY = np.zeros(numSampleGroups)
            widthE = np.zeros(numSampleGroups)
        # pad for Fortran call
        widthY = PadArray(widthY, 51)
        widthE = PadArray(widthE, 51)

        return widthY, widthE

    def C2Fw(self, sname):
        output_workspace = sname + '_Result'
        num_spectra = 0
        axis_names = []
        x, y, e = [], [], []
        for nl in range(1, 4):
            num_params = nl * 3 + 1
            num_spectra += num_params

            amplitude_data, width_data = [], []
            amplitude_error, width_error = [], []

            # read data from file output by fortran code
            file_name = sname + '.ql' + str(nl)
            x_data, peak_data, peak_error = self._read_ql_file(file_name, nl)
            x_data = np.asarray(x_data)

            amplitude_data, width_data, height_data = peak_data
            amplitude_error, width_error, height_error = peak_error

            # transpose y and e data into workspace rows
            amplitude_data, width_data = np.asarray(amplitude_data).T, np.asarray(width_data).T
            amplitude_error, width_error = np.asarray(amplitude_error).T, np.asarray(width_error).T
            height_data, height_error = np.asarray(height_data), np.asarray(height_error)

            # calculate EISF and EISF error
            total = height_data + amplitude_data
            EISF_data = height_data / total
            total_error = height_error ** 2 + amplitude_error ** 2
            EISF_error = EISF_data * np.sqrt((height_error ** 2 / height_data ** 2) + (total_error / total ** 2))

            # interlace amplitudes and widths of the peaks
            y.append(np.asarray(height_data))
            for amp, width, EISF in zip(amplitude_data, width_data, EISF_data):
                y.append(amp)
                y.append(width)
                y.append(EISF)

            # interlace amplitude and width errors of the peaks
            e.append(np.asarray(height_error))
            for amp, width, EISF in zip(amplitude_error, width_error, EISF_error):
                e.append(amp)
                e.append(width)
                e.append(EISF)

            # create x data and axis names for each function
            axis_names.append('f' + str(nl) + '.f0.' + 'Height')
            x.append(x_data)
            for j in range(1, nl + 1):
                axis_names.append('f' + str(nl) + '.f' + str(j) + '.Amplitude')
                x.append(x_data)
                axis_names.append('f' + str(nl) + '.f' + str(j) + '.FWHM')
                x.append(x_data)
                axis_names.append('f' + str(nl) + '.f' + str(j) + '.EISF')
                x.append(x_data)

        x = np.asarray(x).flatten()
        y = np.asarray(y).flatten()
        e = np.asarray(e).flatten()

        s_api.CreateWorkspace(OutputWorkspace=output_workspace, DataX=x, DataY=y, DataE=e, Nspec=num_spectra,
                              UnitX='MomentumTransfer', YUnitLabel='', VerticalAxisUnit='Text',
                              VerticalAxisValues=axis_names)

        return output_workspace

    def _yield_floats(self, block):
        # yield a list of floats from a list of lines of text
        # encapsulates the iteration over a block of lines
        for line in block:
            yield ExtractFloat(line)

    def _read_ql_file(self, file_name, nl):
        # offset to ignore header
        header_offset = 8
        block_size = 4 + nl * 3

        asc = self._read_ascii_file(file_name)
        # extract number of blocks from the file header
        num_blocks = int(ExtractFloat(asc[3])[0])

        q_data = []
        amp_data, FWHM_data, height_data = [], [], []
        amp_error, FWHM_error, height_error = [], [], []

        # iterate over each block of fit parameters in the file
        # each block corresponds to a single column in the final workspace
        for block_num in range(num_blocks):
            lower_index = header_offset + (block_size * block_num)
            upper_index = lower_index + block_size

            # create iterator for each line in the block
            line_pointer = self._yield_floats(asc[lower_index:upper_index])

            # Q,AMAX,HWHM,BSCL,GSCL
            line = next(line_pointer)
            Q, AMAX, HWHM, _, _ = line
            q_data.append(Q)

            # A0,A1,A2,A4
            line = next(line_pointer)
            block_height = AMAX * line[0]

            # parse peak data from block
            block_FWHM = []
            block_amplitude = []
            for _ in range(nl):
                # Amplitude,FWHM for each peak
                line = next(line_pointer)
                amp = AMAX * line[0]
                FWHM = 2. * HWHM * line[1]
                block_amplitude.append(amp)
                block_FWHM.append(FWHM)

            # next parse error data from block
            # SIG0
            line = next(line_pointer)
            block_height_e = line[0]

            block_FWHM_e = []
            block_amplitude_e = []
            for _ in range(nl):
                # Amplitude error,FWHM error for each peak
                # SIGIK
                line = next(line_pointer)
                amp = AMAX * math.sqrt(math.fabs(line[0]) + 1.0e-20)
                block_amplitude_e.append(amp)

                # SIGFK
                line = next(line_pointer)
                FWHM = 2.0 * HWHM * math.sqrt(math.fabs(line[0]) + 1.0e-20)
                block_FWHM_e.append(FWHM)

            # append data from block
            amp_data.append(block_amplitude)
            FWHM_data.append(block_FWHM)
            height_data.append(block_height)

            # append error values from block
            amp_error.append(block_amplitude_e)
            FWHM_error.append(block_FWHM_e)
            height_error.append(block_height_e)

        return q_data, (amp_data, FWHM_data, height_data), (amp_error, FWHM_error, height_error)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(BayesQuasi)
