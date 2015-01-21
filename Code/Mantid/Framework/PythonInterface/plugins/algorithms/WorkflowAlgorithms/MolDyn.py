from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

import os
import numpy as np
import math


def _split_line(a):
    elements = a.split()  # split line on character
    extracted = []
    for n in elements:
        extracted.append(float(n))
    return extracted  # values as list

def _find_starts(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l]
        if char.startswith(c):
            line = l
            break
    return line

def _find_tab_starts(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l][1:]
        if char.startswith(c):
            line = l
            break
    return line

def _find_ends(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l]
        if char.endswith(c):
            line = l
            break
    return line

def _find_char(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l]
        if char.find(c):
            line = l
            break
    return line

def _make_list(a, l1, l2):
    data = ''
    for m in range(l1, l2 + 1):
        data += a[m]
        alist = data.split(',')
    return alist


class MolDyn(PythonAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'

    def summary(self):
        return 'Imports nMOLDYN simulations from CDL and ASCII files.'

    def PyInit(self):
        self.declareProperty(FileProperty('Filename', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.cdl', '.dat']),
                                          doc='File path for data')

        self.declareProperty(StringArrayProperty('Functions'),
                             doc='The Function to use')

        self.declareProperty(WorkspaceProperty('Resolution', '', Direction.Input, PropertyMode.Optional),
                             doc='Resolution workspace')

        self.declareProperty(name='MaxEnergy', defaultValue='',
                             doc='Crop the result spectra at a given energy (leave blank for no crop)')

        self.declareProperty(name='SymmetriseEnergy', defaultValue=False,
                             doc='Symmetrise functions in energy about x=0')

        self.declareProperty(name='Plot', defaultValue='None',
                             validator=StringListValidator(['None', 'Spectra', 'Contour', 'Both']),
                             doc='Plot result workspace')

        self.declareProperty(name='Verbose', defaultValue=False,
                             doc='Output more verbose message to log')

        self.declareProperty(name='Save', defaultValue=False,
                             doc='Save result workspace to nexus file in the default save directory')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc='Output workspace name')


    def validateInputs(self):
        issues = dict()

        sample_filename = self.getPropertyValue('Filename')
        function_list = self.getProperty('Functions').value
        symm = self.getProperty('SymmetriseEnergy').value
        res_ws = self.getPropertyValue('Resolution')
        e_max = self.getPropertyValue('MaxEnergy')

        if len(function_list) == 0 and os.path.splitext(sample_filename)[1] == 'cdl':
            issues['Functions'] = 'Must specify at least one function when loading a CDL file'

        if len(function_list) > 0 and os.path.splitext(sample_filename)[1] == 'dat':
            issues['Functions'] = 'Cannot specify functions when loading an ASCII file'

        if res_ws is not '' and e_max is '':
            issues['MaxEnergy'] = 'MaxEnergy must be set when convolving with an instrument resolution'

        if res_ws is not '' and not symm:
            issues['SymmetriseEnergy'] = 'Must symmetrise energy when convolving with instrument resolution'

        return issues


    def PyExec(self):
        from IndirectImport import import_mantidplot

        self._mtd_plot = import_mantidplot()

        # Do setup
        self._setup()

        try:
            # Load data file
            data, name, ext = self._load_file()

            # Run nMOLDYN import
            if ext == 'cdl':
                self._cdl_import(data, name)
            elif ext == 'dat':
                self._ascii_import(data, name)
            else:
                raise RuntimeError('Unrecognised file format: %s' % ext)

        except Exception as ex:
            logger.error('Error parsing file %s, error is: %s' % (self._sam_path, str(ex)))

        # Do processing specific to workspaces in energy
        if isinstance(mtd[self._out_ws], WorkspaceGroup):
            for ws_name in mtd[self._out_ws].getNames():
                if mtd[ws_name].getAxis(0).getUnit().unitID() == 'Energy':
                    # Get an XMax value, default to max energy if not cropping
                    e_max = mtd[ws_name].dataX(0).max()
                    logger.debug('Max energy in workspace %s: %f' % (ws_name, e_max))

                    if self._emax is not None:
                        if self._emax > e_max:
                            raise ValueError('MaxEnergy crop is out of energy range for function %s' % ws_name)
                        e_max = self._emax

                    # If we are going to Symmetrise then there is no need to crop
                    # as the Symmetrise algorithm will do this
                    if self._symmetrise:
                        # Symmetrise the sample workspace in x=0
                        Symmetrise(Sample=ws_name, XMin=0, XMax=e_max,
                                   Verbose=self._verbose, Plot=False, Save=False,
                                   OutputWorkspace=ws_name)

                    elif self._emax is not None:
                        CropWorkspace(InputWorkspace=ws_name, OutputWorkspace=ws_name,
                                      XMax=self._emax)

        # Do convolution if given a resolution workspace
        if self._res_ws is not '':
            # Create a workspace with enough spectra for convolution
            num_sample_hist = mtd[self._out_ws].getItem(0).getNumberHistograms()
            resolution_ws = self._create_res_ws(num_sample_hist)

            # Convolve all workspaces in output group
            for ws_name in mtd[self._out_ws].getNames():
                if ws_name.lower().find('sqw') != -1:
                    self._convolve_with_res(resolution_ws, ws_name)
                else:
                    if self._verbose:
                        logger.notice('Ignoring workspace %s in convolution step' % ws_name)

            # Remove the generated resolution workspace
            DeleteWorkspace(resolution_ws)

        # Save result workspace group
        if self._save:
            workdir = config['defaultsave.directory']
            out_filename = os.path.join(workdir, self._out_ws + '.nxs')
            if self._verbose:
                logger.information('Creating file: %s' % out_filename)
                SaveNexus(InputWorkspace=self._out_ws, Filename=out_filename)

        # Set the output workspace
        self.setProperty('OutputWorkspace', self._out_ws)

        # Plot spectra plots
        if self._plot == 'Spectra' or self._plot == 'Both':
            if isinstance(mtd[self._out_ws], WorkspaceGroup):
                for ws_name in mtd[self._out_ws].getNames():
                    self._plot_spectra(ws_name)
            else:
                self._plot_spectra(self._out_ws)

        # Plot contour plot
        if self._plot == 'Contour' or self._plot == 'Both':
            self._mtd_plot.plot2D(self._out_ws)


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._verbose = self.getProperty('Verbose').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        self._sam_path = self.getPropertyValue('Filename')
        self._symmetrise = self.getProperty('SymmetriseEnergy').value

        raw_functions = self.getProperty('Functions').value
        self._functions = [x.strip() for x in raw_functions]

        emax_str = self.getPropertyValue('MaxEnergy')
        self._emax = None
        if emax_str != '':
            self._emax = float(emax_str)

        self._res_ws = self.getPropertyValue('Resolution')
        self._out_ws = self.getPropertyValue('OutputWorkspace')


    def _load_file(self):
        """
        Attempts to load the sample file.

        @returns A tuple with the ASCII data, sample file name and file extension
        """

        # Get some data about the file
        path = self._sam_path
        base = os.path.basename(path)
        name = os.path.splitext(base)[0]
        ext = os.path.splitext(path)[1]

        # Remove dot from extension
        if len(ext) > 1:
            ext = ext[1:]

        if self._verbose:
            logger.information('Base filename for %s is %s' % (self._sam_path, name))
            logger.information('File type of %s is %s' % (self._sam_path, ext))

        if not os.path.isfile(path):
            path = FileFinder.getFullPath(path)

        if self._verbose:
            logger.information('Got file path for %s: %s' % (self._sam_path, path))

        # Open file and get data
        try:
            handle = open(path, 'r')
            data = []
            for line in handle:
                line = line.rstrip()
                data.append(line)
            handle.close()

            return data, name, ext

        except:
            raise RuntimeError('Could not load file: %s' % path)


    def _find_dimensions(self, data):
        """
        Gets the number of Q, time and frequency values in given raw data.

        @param data Raw data to search
        """

        num_q_values = _find_tab_starts(data, 'NQVALUES', 0)
        num_time_values = _find_tab_starts(data, 'NTIMES', 0)
        num_freq_values = _find_tab_starts(data, 'NFREQUENCIES', 0)

        q_el = data[num_q_values].split()
        num_q = int(q_el[2])
        t_el = data[num_time_values].split()
        num_t = int(t_el[2])
        f_el = data[num_freq_values].split()
        num_f = int(f_el[2])

        if self._verbose:
            logger.debug(data[2][1:-1])
            logger.debug(data[3][1:-1])
            logger.debug(data[6][1:-1])

        return num_q, num_t, num_f


    def _cdl_import(self, data, name):
        """
        Import data from CDL file.

        @param data Raw data
        @param name Name of data file
        """

        if self._verbose:
            logger.notice('Loading CDL file: %s' % name)

        len_data = len(data)

        # raw head
        nQ, nT, nF = self._find_dimensions(data)
        ldata = _find_starts(data, 'data:', 0)
        lq1 = _find_starts(data, ' q =', ldata)  # start Q values
        lq2 = _find_starts(data, ' q =', lq1 - 1)
        Qlist = _make_list(data, lq1, lq2)
        if nQ != len(Qlist):
            raise RUntimeError('Error reading Q values')
        Qf = Qlist[0].split()
        Q = [float(Qf[2]) / 10.0]
        for m in range(1, nQ - 1):
            Q.append(float(Qlist[m]) / 10.0)

        Q.append(float(Qlist[nQ - 1][:-1]) / 10.0)
        if self._verbose:
            logger.information('Q values = ' + str(Q))

        lt1 = _find_starts(data, ' time =', lq2)  # start T values
        lt2 = _find_ends(data, ';', lt1)
        Tlist = _make_list(data, lt1, lt2)
        if nT != len(Tlist):
            raise RuntimeError('Error reading Time values')

        Tf = Tlist[0].split()
        T = [float(Tf[2])]
        for m in range(1, nT - 1):
            T.append(float(Tlist[m]))

        T.append(float(Tlist[nT - 1][:-1]))
        T.append(2 * T[nT - 1] - T[nT - 2])
        if self._verbose:
            logger.information('T values = ' + str(T[:2]) + ' to ' + str(T[-3:]))

        lf1 = _find_starts(data, ' frequency =', lq2)  # start F values
        lf2 = _find_ends(data, ';', lf1)
        Flist = _make_list(data, lf1, lf2)
        if nF != len(Flist):
            raise RuntimeError('Error reading Freq values')

        Ff = Flist[0].split()
        F = [float(Ff[2])]
        for m in range(1, nF - 1):
            F.append(float(Flist[m]))

        F.append(float(Flist[nF - 1][:-1]))
        F.append(2 * F[nF - 1] - T[nF - 2])
        if self._verbose:
            logger.information('F values = ' + str(F[:2]) + ' to ' + str(F[-3:]))

        # Function
        output_ws_list = list()
        for func in self._functions:
            start = []
            lstart = lt2
            if func[:3] == 'Fqt':
                nP = nT
                xEn = np.array(T)
                eZero = np.zeros(nT)
                xUnit = 'TOF'
            elif func[:3] == 'Sqw':
                nP = nF
                xEn = np.array(F)
                eZero = np.zeros(nF)
                xUnit = 'Energy'
            else:
                raise RuntimeError('Failed to parse function string ' + func)

            for n in range(0, nQ):
                for m in range(lstart, len_data):
                    char = data[m]
                    if char.startswith('  // ' + func):
                        start.append(m)
                        lstart = m + 1
            lend = _find_ends(data, ';', lstart)
            start.append(lend + 1)

            # Throw error if we couldn't find the function
            if len(start) < 2:
                raise RuntimeError('Failed to parse function string ' + func)

            Qaxis = ''
            for n in range(0, nQ):
                if self._verbose:
                    logger.information(str(start))
                    logger.information('Reading : ' + data[start[n]])

                Slist = _make_list(data, start[n] + 1, start[n + 1] - 1)
                if n == nQ - 1:
                    Slist[nP - 1] = Slist[nP - 1][:-1]
                S = []
                for m in range(0, nP):
                    S.append(float(Slist[m]))
                if nP != len(S):
                    raise RuntimeError('Error reading S values')
                else:
                    if self._verbose:
                        logger.information('S values = ' + str(S[:2]) + ' to ' + str(S[-2:]))
                if n == 0:
                    Qaxis += str(Q[n])
                    xDat = xEn
                    yDat = np.array(S)
                    eDat = eZero
                else:
                    Qaxis += ',' + str(Q[n])
                    xDat = np.append(xDat, xEn)
                    yDat = np.append(yDat, np.array(S))
                    eDat = np.append(eDat, eZero)
            outWS = name + '_' + func
            CreateWorkspace(OutputWorkspace=outWS, DataX=xDat, DataY=yDat, DataE=eDat,
                            Nspec=nQ, UnitX=xUnit, VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
            output_ws_list.append(outWS)

        GroupWorkspaces(InputWorkspaces=output_ws_list, OutputWorkspace=self._out_ws)


    def _ascii_import(self, data, name):
        """
        Import ASCII data.

        @param data Raw ASCII data
        @param name Name of data file
        """

        from IndirectNeutron import ChangeAngles, InstrParas, RunParas

        if self._verbose:
            logger.notice('Loading ASCII data: %s' % name)

        val = _split_line(data[3])
        Q = []
        for n in range(1, len(val)):
            Q.append(val[n])

        nQ = len(Q)
        x = []
        y = []
        for n in range(4, len(data)):
            val = _split_line(data[n])
            x.append(val[0])
            yval = val[1:]
            y.append(yval)

        nX = len(x)
        if self._verbose:
            logger.information('nQ = ' + str(nQ))
            logger.information('nT = ' + str(nX))

        xT = np.array(x)
        eZero = np.zeros(nX)
        Qaxis = ''
        for m in range(0, nQ):
            if self._verbose:
                logger.information('Q[' + str(m + 1) + '] : ' + str(Q[m]))

            S = []
            for n in range(0, nX):
                S.append(y[n][m])

            if m == 0:
                Qaxis += str(Q[m])
                xDat = xT
                yDat = np.array(S)
                eDat = eZero
            else:
                Qaxis += ',' + str(Q[m])
                xDat = np.append(xDat, xT)
                yDat = np.append(yDat, np.array(S))
                eDat = np.append(eDat, eZero)

        CreateWorkspace(OutputWorkspace=self._out_ws, DataX=xDat, DataY=yDat, DataE=eDat,
                        Nspec=nQ, UnitX='TOF')
        Qmax = Q[nQ - 1]
        instr = 'MolDyn'
        ana = 'qmax'
        if Qmax <= 2.0:
            refl = '2'
        else:
            refl = '4'

        InstrParas(self._out_ws, instr, ana, refl)
        efixed = RunParas(self._out_ws, instr, name, name, self._verbose)
        if self._verbose:
            logger.information('Qmax = ' + str(Qmax) + ' ; efixed = ' + str(efixed))
        pi4 = 4.0 * math.pi
        wave = 1.8 * math.sqrt(25.2429 / efixed)
        theta = []
        for n in range(0, nQ):
            qw = wave * Q[n] / pi4
            ang = 2.0 * math.degrees(math.asin(qw))
            theta.append(ang)

        ChangeAngles(self._out_ws, instr, theta, self._verbose)


    def _create_res_ws(self, num_sample_hist):
        """
        Creates a resolution workspace.

        @param num_sample_hist Number of histgrams required in workspace
        @returns The generated resolution workspace
        """

        num_res_hist = mtd[self._res_ws].getNumberHistograms()

        if self._verbose:
            logger.notice('Creating resolution workspace.')
            logger.information('Sample has %d spectra\nResolution has %d spectra'
                               % (num_sample_hist, num_res_hist))

        # If the sample workspace has more spectra than the resolution then copy the first spectra
        # to make a workspace with equal spectra count to sample
        if num_sample_hist > num_res_hist:
            if self._verbose:
                logger.information('Copying first resolution spectra for convolution')

            res_ws_list = []
            for _ in range(0, num_sample_hist):
                res_ws_list.append(self._res_ws)

            res_ws_str_list = ','.join(res_ws_list)
            resolution_ws = ConjoinSpectra(res_ws_str_list, 0)

        # If sample has less spectra then crop the resolution to the same number of spectra as
        # resolution
        elif num_sample_hist < num_res_hist:
            if self._verbose:
                logger.information('Cropping resolution workspace to sample')

            resolution_ws = CropWorkspace(InputWorkspace=self._res_ws, StartWorkspaceIndex=0,
                                          EndWorkspaceIndex=num_sample_hist)

        # If the spectra counts match then just use the resolution as it is
        else:
            if self._verbose:
                logger.information('Using resolution workspace as is')

            resolution_ws = CloneWorkspace(self._res_ws)

        return resolution_ws


    def _convolve_with_res(self, resolution_ws, function_ws_name):
        """
        Performs convolution with an instrument resolution workspace.

        @param resolution_ws The resolution workspace to convolve with
        @param function_ws_name The workspace name for the function to convolute
        """

        if self._verbose:
            logger.notice('Convoluting sample %s with resolution %s'
                          % (function_ws_name, resolution_ws))

        # Convolve the symmetrised sample with the resolution
        ConvolveWorkspaces(OutputWorkspace=function_ws_name,
                           Workspace1=function_ws_name, Workspace2=resolution_ws)


    def _plot_spectra(self, ws_name):
        """
        Plots up to the first 10 spectra from a workspace.

        @param ws_name Name of workspace to plot
        """

        num_hist = mtd[ws_name].getNumberHistograms()

        # Limit number of plotted histograms to 10
        if num_hist > 10:
            num_hist = 10

        # Build plot list
        plot_list = []
        for i in range(0, num_hist):
            plot_list.append(i)

        self._mtd_plot.plotSpectrum(ws_name, plot_list)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MolDyn)
