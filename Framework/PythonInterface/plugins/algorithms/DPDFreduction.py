# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import os
import numpy
import re
import mantid.simpleapi as sapi
import mantid.api as api
import mantid.kernel as kapi
from mantid import config
from tempfile import mkstemp

# conversion factor from energy (in meV) to vawevector (in inverse Angstroms)
ENERGY_TO_WAVEVECTOR = 2.072

#pylint: disable=too-many-instance-attributes


class DPDFreduction(api.PythonAlgorithm):
    channelgroup = None

    _runs = None
    _vanfile = None
    _ecruns = None
    _ebins = None
    _qbins = None
    _snorm = None
    _clean = None

    def category(self):
        return "Inelastic\\Reduction"

    def name(self):
        return 'DPDFreduction'

    def summary(self):
        return 'Calculate S(Q,E) from powder or isotropic data'

    def PyInit(self):

        # Input parameters
        titleInputOptions = "Input"
        self.declareProperty('RunNumbers', '', 'Sample run numbers')
        self.setPropertyGroup("RunNumbers", titleInputOptions)
        self.declareProperty(api.FileProperty(name='Vanadium', defaultValue='',
                                              action=api.FileAction.OptionalLoad,
                                              extensions=['.nxs']),
                             'Preprocessed white-beam vanadium file.')
        self.setPropertyGroup("Vanadium", titleInputOptions)
        self.declareProperty('EmptyCanRunNumbers', '', 'Empty can run numbers')
        self.setPropertyGroup("EmptyCanRunNumbers", titleInputOptions)

        # Configuration parameters
        titleConfigurationOptions = "Configuration"

        e_validator = kapi.FloatArrayLengthValidator(1,3)
        self.declareProperty(kapi.FloatArrayProperty('EnergyBins', [1.5],
                                                     validator=e_validator),
                             'Energy transfer binning scheme (in meV)')
        self.setPropertyGroup("EnergyBins", titleConfigurationOptions)

        q_validator = kapi.FloatArrayLengthValidator(0, 3)
        self.declareProperty(kapi.FloatArrayProperty('MomentumTransferBins',
                                                     list(),
                                                     validator=q_validator),
                             'Momentum transfer binning scheme (in inverse Angstroms)')
        self.setPropertyGroup("MomentumTransferBins", titleConfigurationOptions)

        self.declareProperty('NormalizeSlices', False,
                             'Do we normalize each slice?',
                             direction=kapi.Direction.Input)
        self.setPropertyGroup("NormalizeSlices", titleConfigurationOptions)

        # Ouptut parameters
        titleOuptutOptions = "Output"
        self.declareProperty('CleanWorkspaces', True,
                             'Do we clean intermediate steps?',
                             direction=kapi.Direction.Input)
        self.setPropertyGroup("CleanWorkspaces", titleOuptutOptions)
        self.declareProperty(api.MatrixWorkspaceProperty('OutputWorkspace',
                                                         'S_Q_E_sliced',
                                                         kapi.Direction.Output),
                             "Output workspace")
        self.setPropertyGroup("OutputWorkspace", titleOuptutOptions)

    def validateInputs(self):
        issues = dict()

        # Pattern validator for run numbers
        def checkrunnumbers(runs_property_name):
            runs_string = self.getPropertyValue(runs_property_name)
            regexp_pattern = re.compile(r'^\s*\d+\s*$|^\s*\d+\s*-\s*\d+\s*$|^\s*[\d++]+\d+\s*$')
            if not regexp_pattern.match(runs_string):
                issues[runs_property_name] = runs_property_name + ' allowed syntaxes:\n' + \
                                             '(1)Single number\n' + \
                                             '(2) A continuous range of numbers, like 5123-5130\n' + \
                                             '(3)Addition of numbers, like 5123+5128+5130'
            elif re.compile(r'^\s*\d+\s*-\s*\d+\s*$').match(runs_string):
                first, second = [int(x) for x in runs_string.split('-')]
                if first >= second:
                    issues[runs_property_name] = runs_property_name +\
                                                 ' should be increasing'

        # check syntax for RunNumbers
        checkrunnumbers('RunNumbers')
        # check syntax for EmptyCanRunNumbers
        if self.getPropertyValue('EmptyCanRunNumbers'):
            checkrunnumbers('EmptyCanRunNumbers')
        # check energy bins
        ebins = self.getProperty('EnergyBins').value
        if len(ebins) not in (1, 3):
            issues['EnergyBins'] = 'Energy bins is a list of either one or three values'
        # check momentum transfer bins
        qbins = self.getProperty('MomentumTransferBins').value
        if len(qbins) not in (0, 1, 3):
            issues['MomentumTransferBins'] =\
                'Momentum transfer bins is a list of zero (empty list), one, or three values'
        return issues

    #pylint: disable=too-many-locals, too-many-branches
    def PyExec(self): # noqa
        self._runs = self.getProperty('RunNumbers').value
        self._vanfile = self.getProperty('Vanadium').value
        self._ecruns = self.getProperty('EmptyCanRunNumbers').value
        self._ebins = (self.getProperty('EnergyBins').value).tolist()
        self._qbins = (self.getProperty('MomentumTransferBins').value).tolist()
        self._snorm = self.getProperty('NormalizeSlices').value
        self._clean = self.getProperty('CleanWorkspaces').value
        wn_sqes = self.getPropertyValue("OutputWorkspace")

        # workspace names
        prefix = ''
        if self._clean:
            prefix = '__'
        # "wn" denotes workspace name
        wn_data = prefix + 'data'  # Accumulated data events
        wn_data_mon = prefix + 'data_monitors'  # Accumulated monitors for data
        wn_van = prefix + 'vanadium'  # White-beam vanadium
        wn_van_st = prefix + 'vanadium_S_theta'
        wn_reduced = prefix + 'reduced'  # data after DGSReduction
        wn_ste = prefix + 'S_theta_E'  # data after grouping by theta angle
        wn_sten = prefix + 'S_theta_E_normalized'
        wn_steni = prefix + 'S_theta_E_interp'
        wn_sqe = prefix + 'S_Q_E'
        wn_sqeb = prefix + 'S_Q_E_binned'
        wn_sqesn = prefix + wn_sqes + '_norm'
        # Empty can files
        wn_ec_data = prefix + 'ec_data'  # Accumulated empty can data
        wn_ec_data_mon = prefix + 'ec_data_monitors'  # Accumulated monitors for empty can
        wn_ec_reduced = prefix + 'ec_reduced'  # empty can data after DGSReduction
        wn_ec_ste = prefix + 'ec_S_theta_E'  # empty can data after grouping by theta angle

        # Save current configuration
        facility = config['default.facility']
        instrument = config['default.instrument']
        datasearch = config["datasearch.searcharchive"]
        # Allows searching for ARCS run numbers
        config['default.facility'] = 'SNS'
        config['default.instrument'] = 'ARCS'
        config["datasearch.searcharchive"] = "On"

        try:
            # Load the vanadium file, assumed to be preprocessed, meaning that
            # for every detector all events within a particular wide wavelength
            # range have been rebinned into a single histogram
            self._load(self._vanfile, wn_van)
            # Check for white-beam vanadium, true if the vertical chopper is absent (vChTrans==2)
            if api.mtd[wn_van].run().getProperty('vChTrans').value[0] != 2:
                raise ValueError("White-vanadium is required")

            # Load several event files into a single workspace. The nominal incident
            # energy should be the same to avoid difference in energy resolution
            self._load(self._runs, wn_data)

            # Load empty can event files, if present
            if self._ecruns:
                self._load(self._ecruns, wn_ec_data)

        finally:
            # Recover the default configuration
            config['default.facility'] = facility
            config['default.instrument'] = instrument
            config["datasearch.searcharchive"] = datasearch

        # Obtain incident energy as the mean of the nominal Ei values.
        # There is one nominal value for each run number.
        ws_data = sapi.mtd[wn_data]
        Ei = ws_data.getRun()['EnergyRequest'].getStatistics().mean
        Ei_std = ws_data.getRun()['EnergyRequest'].getStatistics().standard_deviation

        # Verify empty can runs were obtained at similar energy
        if self._ecruns:
            ws_ec_data = sapi.mtd[wn_ec_data]
            ec_Ei = ws_ec_data.getRun()['EnergyRequest'].getStatistics().mean
            if abs(Ei - ec_Ei) > Ei_std:
                raise RuntimeError('Empty can runs were obtained at a significant' +
                                   ' different incident energy than the sample runs')

        # Obtain energy range. If user did not supply a triad
        # [Estart, Ewidth, Eend] but only Ewidth, then estimate
        # Estart and End from the nominal energies
        if len(self._ebins) == 1:
            ws_data = sapi.mtd[wn_data]
            Ei = ws_data.getRun()['EnergyRequest'].getStatistics().mean
            self._ebins.insert(0, -0.5 * Ei)  # prepend
            self._ebins.append(0.95 * Ei)  # append

        # Enforce that the elastic energy (E=0) lies in the middle of the
        # central bin with an appropriate small shift in the energy range
        Ei_min_reduced = self._ebins[0] / self._ebins[1]
        remainder = Ei_min_reduced - int(Ei_min_reduced)
        if remainder >= 0.0:
            erange_shift = self._ebins[1] * (0.5 - remainder)
        else:
            erange_shift = self._ebins[1] * (-0.5 - remainder)
        self._ebins[0] += erange_shift  # shift minimum energy
        self._ebins[-1] += erange_shift  # shift maximum energy

        # Convert to energy transfer. Normalize by proton charge.
        # The output workspace is S(detector-id,E)
        factor = 0.1  # use a finer energy bin than the one passed (self._ebins[1])
        Erange = '{0},{1},{2}'.format(self._ebins[0], factor * self._ebins[1], self._ebins[2])
        Ei_calc, T0 = sapi.GetEiT0atSNS(MonitorWorkspace=wn_data_mon, IncidentEnergyGuess=Ei)
        sapi.MaskDetectors(Workspace=wn_data, MaskedWorkspace=wn_van)  # Use vanadium mask
        sapi.DgsReduction(SampleInputWorkspace=wn_data,
                          SampleInputMonitorWorkspace=wn_data_mon,
                          IncidentEnergyGuess=Ei_calc,
                          UseIncidentEnergyGuess=1,
                          TimeZeroGuess=T0,
                          EnergyTransferRange=Erange,
                          IncidentBeamNormalisation='ByCurrent',
                          OutputWorkspace=wn_reduced)

        if self._ecruns:
            sapi.MaskDetectors(Workspace=wn_ec_data, MaskedWorkspace=wn_van)
            sapi.DgsReduction(SampleInputWorkspace=wn_ec_data,
                              SampleInputMonitorWorkspace=wn_ec_data_mon,
                              IncidentEnergyGuess=Ei_calc,
                              UseIncidentEnergyGuess=1,
                              TimeZeroGuess=T0,
                              EnergyTransferRange=Erange,
                              IncidentBeamNormalisation='ByCurrent',
                              OutputWorkspace=wn_ec_reduced)

        # Obtain maximum and minimum |Q| values, as well as dQ if none passed
        if len(self._qbins) < 3:
            if not self._qbins:
                # insert dQ if empty qbins. The minimal momentum transfer
                # is the result on an event where the initial energy was
                # Ei and the final energy was Ei+dE.
                dE = self._ebins[1]
                self._qbins.append(numpy.sqrt((Ei + dE) / ENERGY_TO_WAVEVECTOR) -
                                   numpy.sqrt(Ei / ENERGY_TO_WAVEVECTOR))
            mins, maxs = sapi.ConvertToMDMinMaxLocal(wn_reduced, Qdimensions='|Q|',
                                                     dEAnalysisMode='Direct')
            self._qbins.insert(0, mins[0])  # prepend minimum Q
            self._qbins.append(maxs[0])  # append maximum Q

        # Delete sample and empty can event workspaces to free memory.
        if self._clean:
            sapi.DeleteWorkspace(wn_data)
            if self._ecruns:
                sapi.DeleteWorkspace(wn_ec_data)

        # Convert to S(theta,E)
        ki = numpy.sqrt(Ei / ENERGY_TO_WAVEVECTOR)
        # If dE is the smallest energy transfer considered,
        # then dQ/ki is the smallest dtheta (in radians)
        dtheta = self._qbins[1] / ki * (180.0 / numpy.pi)
        # Use a finer dtheta that the nominal smallest value
        factor = 1. / 5  # a reasonable (heuristic) value
        dtheta *= factor
        # Fix: a very small dtheta (<0.15 degrees) prevents correct interpolation
        dtheta = max(0.15, dtheta)
        # Group detectors according to theta angle for the sample runs
        group_file_os_handle, group_file_name = mkstemp(suffix='.xml')
        group_file_handle = os.fdopen(group_file_os_handle, 'w')
        sapi.GenerateGroupingPowder(InputWorkspace=wn_reduced, AngleStep=dtheta,
                                    GroupingFilename=group_file_name)
        group_file_handle.close()
        sapi.GroupDetectors(InputWorkspace=wn_reduced, MapFile=group_file_name,
                            OutputWorkspace=wn_ste)
        # Group detectors according to theta angle for the emtpy can run
        if self._ecruns:
            sapi.GroupDetectors(InputWorkspace=wn_ec_reduced, MapFile=group_file_name,
                                OutputWorkspace=wn_ec_ste)
            # Subtract the empty can from the can+sample
            sapi.Minus(LHSWorkspace=wn_ste, RHSWorkspace=wn_ec_ste, OutputWorkspace=wn_ste)

        # Normalize by the vanadium intensity, but before that we need S(theta)
        # for the vanadium. Recall every detector has all energies into a single
        # bin, so we get S(theta) instead of S(theta,E)
        sapi.GroupDetectors(InputWorkspace=wn_van, MapFile=group_file_name,
                            OutputWorkspace=wn_van_st)
        # Divide by vanadium. Make sure it is integrated in the energy domain
        sapi.Integration(wn_van_st, OutputWorkspace=wn_van_st)
        sapi.Divide(wn_ste, wn_van_st, OutputWorkspace=wn_sten)
        sapi.ClearMaskFlag(Workspace=wn_sten)

        # Temporary file generated by GenerateGroupingPowder to be removed
        os.remove(group_file_name)  # no need for this file
        os.remove(os.path.splitext(group_file_name)[0]+".par")

        max_i_theta = 0.0
        min_i_theta = 0.0

        # Linear interpolation for those theta values with low intensity
        # First, find minimum theta index with a non-zero histogram
        ws_sten = sapi.mtd[wn_sten]

        for i_theta in range(ws_sten.getNumberHistograms()):
            if ws_sten.dataY(i_theta).any():
                min_i_theta = i_theta
                break
        # second, find maximum theta with a non-zero histogram
        for i_theta in range(ws_sten.getNumberHistograms() - 1, -1, -1):
            if ws_sten.dataY(i_theta).any():
                max_i_theta = i_theta
                break

        # Scan a range of theta angles and apply interpolation to those theta angles
        # with considerably low intensity (gaps)
        delta_theta = max_i_theta - min_i_theta
        gaps = self._findGaps(wn_sten, int(min_i_theta+0.1*delta_theta), int(max_i_theta-0.1*delta_theta))
        sapi.CloneWorkspace(InputWorkspace=wn_sten, OutputWorkspace=wn_steni)
        for gap in gaps:
            self._interpolate(wn_steni, gap)  # interpolate this gap

        # Convert S(theta,E) to S(Q,E), then rebin in |Q| and E to MD workspace
        sapi.ConvertToMD(InputWorkspace=wn_steni, QDimensions='|Q|',
                         dEAnalysisMode='Direct', OutputWorkspace=wn_sqe)
        Qmin = self._qbins[0]
        Qmax = self._qbins[-1]
        dQ = self._qbins[1]
        Qrange = '|Q|,{0},{1},{2}'.format(Qmin, Qmax, int((Qmax - Qmin) / dQ))
        Ei_min = self._ebins[0]
        Ei_max = self._ebins[-1]
        dE = self._ebins[1]
        deltaErange = 'DeltaE,{0},{1},{2}'.format(Ei_min, Ei_max, int((Ei_max - Ei_min) / dE))
        sapi.BinMD(InputWorkspace=wn_sqe, AxisAligned=1, AlignedDim0=Qrange,
                   AlignedDim1=deltaErange, OutputWorkspace=wn_sqeb)

        # Slice the data by transforming to a Matrix2Dworkspace,
        # with deltaE along the vertical axis
        sapi.ConvertMDHistoToMatrixWorkspace(InputWorkspace=wn_sqeb,
                                             Normalization='NumEventsNormalization',
                                             OutputWorkspace=wn_sqes)

        # Ensure correct units
        sapi.mtd[wn_sqes].getAxis(0).setUnit("MomentumTransfer")
        sapi.mtd[wn_sqes].getAxis(1).setUnit("DeltaE")

        # Shift the energy axis, since the reported values should be the center
        # of the bins, instead of the minimum bin boundary
        ws_sqes = sapi.mtd[wn_sqes]
        Eaxis = ws_sqes.getAxis(1)
        e_shift = self._ebins[1] / 2.0
        for i in range(Eaxis.length()):
            Eaxis.setValue(i, Eaxis.getValue(i) + e_shift)

        # Normalize each slice, if requested
        if self._snorm:
            sapi.Integration(InputWorkspace=wn_sqes, OutputWorkspace=wn_sqesn)
            sapi.Divide(LHSWorkspace=wn_sqes, RHSWorkspace=wn_sqesn, OutputWorkspace=wn_sqes)

        # Clean up workspaces from intermediate steps
        if self._clean:
            for name in (wn_van, wn_reduced, wn_ste, wn_van_st, wn_sten,
                         wn_steni, wn_sqe, wn_sqeb, wn_sqesn, 'PreprocessedDetectorsWS'):
                if sapi.mtd.doesExist(name):
                    sapi.DeleteWorkspace(name)

        # Ouput some info as a Notice in the log
        ebins = ', '.join(['{0:.2f}'.format(x) for x in self._ebins])
        qbins = ', '.join(['{0:.2f}'.format(x) for x in self._qbins])
        tbins = '{0:.2f} {1:.2f} {2:.2f}'.format(min_i_theta*dtheta, dtheta, max_i_theta*dtheta)
        message = '\n******  SOME OUTPUT INFORMATION ***' + \
                  '\nEnergy bins: ' + ebins + \
                  '\nQ bins: ' + qbins + \
                  '\nTheta bins: '+tbins
        kapi.logger.notice(message)

        self.setProperty("OutputWorkspace", sapi.mtd[wn_sqes])

    def _load(self, run_numbers, data_name):
        """
        Load data and monitors for run numbers and monitors.
        Algorithm 'Load' can aggregate many runs into a single workspace, but it is not able to do so
        with the monitor workspaces.
        :param run_numbers: run numbers for data event files
        :param data_name: output name for data workspace. The name for the workspace holding the
        monitor data will be data_name+'_monitors'
        :return: None
        """
        # Find out the files for each run
        load_algorithm = api.AlgorithmManager.createUnmanaged("Load")
        load_algorithm.initialize()
        load_algorithm.setPropertyValue('Filename', str(run_numbers))
        files = (load_algorithm.getProperty('Filename').value)[0]
        if not isinstance(files, list):
            # run_numbers represents one file only
            sapi.Load(Filename=files, LoadMonitors=True, OutputWorkspace=data_name)
        else:
            sapi.Load(Filename=files[0], LoadMonitors=True, OutputWorkspace=data_name)
            monitor_name = data_name + '_monitors'
            for file in files[1:]:
                sapi.Load(Filename=file, LoadMonitors=True, OutputWorkspace=data_name+'_tmp')
                sapi.Plus(LHSWorkspace=data_name, RHSWorkspace=data_name+'_tmp', OutputWorkspace=data_name)
                sapi.Plus(LHSWorkspace=monitor_name, RHSWorkspace=data_name + '_tmp_monitors', OutputWorkspace=monitor_name)
            sapi.DeleteWorkspace(data_name+'_tmp')
        if sapi.mtd[data_name].getInstrument().getName() not in ('ARCS'):
            raise NotImplementedError("This algorithm works only for ARCS instrument")

    def _findGaps(self, workspace_name, min_i, max_i):
        """
        Find workspace indexes with a low overall intensity
        A histogram with low intensity contains zero-intensity values for many
        of the energy values (Energy is the X-axis)
        :param workspace_name:
        :param min_i: minimum workspace index to look for
        :param max_i: 1+maximum workspace index to look for
        :return: chunks of consecutive workspace indexes with low overall intensity
        """
        zero_fraction = list()  # for each histogram, count the number of zeros
        workspace = sapi.mtd[workspace_name]
        for index in range(min_i, max_i):
            y = workspace.dataY(index)
            zero_fraction.append(1.0 - (1. * numpy.count_nonzero(y)) / len(y))
        # Find workspace indexes zero fraction above a reasonable threshold
        threshold = numpy.mean(zero_fraction) + 2 * numpy.std(zero_fraction)  # above twice the standard deviation
        high_zero_fraction = min_i + (numpy.where(zero_fraction > threshold))[0]
        # split the high_zero_fraction indexes into chunks of consecutive indexes
        #  Example: if high_zero_fraction=[3,7,8,9,11,15,16], then we split into [3],[7,8,9], [11], [15,16]
        gaps = list()  # intensity gaps, because high zero fraction means low overall intensity
        gap = [numpy.asscalar(high_zero_fraction[0]), ]
        for index in range(1, len(high_zero_fraction)):
            if high_zero_fraction[index] - high_zero_fraction[index - 1] == 1:
                gap.append(numpy.asscalar(high_zero_fraction[index]))  # two consecutive indexes
            else:
                gaps.append(gap)
                gap = [numpy.asscalar(high_zero_fraction[index]), ]
        gaps.append(gap)  # final dangling gap has to be appended
        return gaps  # a list of lists

    def _interpolate(self, workspace_name, gap):
        """
        Assign intensity to the workspace indexes in the gap with the help
        of the adjacent histograms via linear interpolation
        :param workspace_name:
        :param gap: a list of consecutive indexes
        :return:
        """
        nonnull_i_theta_start = gap[0] - 1  # index of adjacent histogram with intensity not low
        nonnull_i_theta_end = gap[-1] + 1  # index of adjacent histogram with intensity not low
        workspace = sapi.mtd[workspace_name]
        y_start = workspace.dataY(nonnull_i_theta_start)
        y_end = workspace.dataY(nonnull_i_theta_end)
        intercept = y_start
        slope = (y_end - y_start) / (nonnull_i_theta_end - nonnull_i_theta_start)
        for null_i_theta in range(1 + nonnull_i_theta_start, nonnull_i_theta_end):
            workspace.dataY(null_i_theta)[:] = \
                intercept + slope * (null_i_theta - nonnull_i_theta_start)  # linear interpolation


# Register algorithm with Mantid.
api.AlgorithmFactory.subscribe(DPDFreduction)
