#pylint: disable=no-init,invalid-name
import os
import numpy
import re
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from mantid import config
from tempfile import mkstemp

# conversion factor from energy (in meV) to vawevector (in inverse Angstroms)
ENERGY_TO_WAVEVECTOR = 2.072

#pylint: disable=too-many-instance-attributes
class DPDFreduction(PythonAlgorithm):
    channelgroup = None

    _runs = None
    _vanfile = None
    _ecruns = None
    _ebins_str = None
    _qbins_str = None
    _snorm = None
    _clean = None
    _qbins = None
    _ebins = None

    def category(self):
        return "Inelastic\\Reduction;Utility\\Development"

    def name(self):
        return 'DPDFreduction'

    def summary(self):
        return 'Obtain S(Q,E) sliced in E'

    def PyInit(self):
        self.declareProperty('RunNumbers', '', 'Sample run numbers')
        self.declareProperty(FileProperty(name='Vanadium', defaultValue='',
                                          action=FileAction.OptionalLoad, extensions=['.nxs']),
                             'Preprocessed vanadium file.')
        self.declareProperty('EmptyCanRunNumbers', '', 'Empty can run numbers')
        self.declareProperty('EnergyBins', '1.5',
                             'Energy transfer binning scheme (in meV)',
                             direction=Direction.Input)
        self.declareProperty('MomentumTransferBins', '',
                             'Momentum transfer binning scheme (in inverse Angstroms)',
                             direction=Direction.Input)
        self.declareProperty('NormalizeSlices', False,
                             'Do we normalize each slice?', direction=Direction.Input)
        self.declareProperty('CleanWorkspaces', True,
                             'Do we clean intermediate steps?', direction=Direction.Input)
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', 'S_Q_E_sliced',
                                                     Direction.Output), "Output workspace")

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
                    issues[runs_property_name] = runs_property_name + ' should be increasing'

        # check syntax for RunNumbers
        checkrunnumbers('RunNumbers')
        # check syntax for EmptyCanRunNumbers
        if self.getPropertyValue('EmptyCanRunNumbers'):
            checkrunnumbers('EmptyCanRunNumbers')
        # check energy bins
        ebins = self.getPropertyValue('EnergyBins')
        pattern = re.compile(r'^\s*\d+[\.\d+]*\s*$|' +
                             r'^\s*\d+[\.\d+]*\s*,\s*\d+[\.\d+]*\s*,\s*\d+[\.\d+]*\s*$')
        if not pattern.match(ebins):
            issues['EnergyBins'] = 'Energy bins is a string containing ' + \
                                   'either one number or a triad separated by commas'
        # check momentum transfer bins
        qbins = self.getPropertyValue('MomentumTransferBins')
        if qbins and not pattern.match(qbins):
            issues['MomentumTransferBins'] = 'Momentum transfer bins is a string ' + \
                                             'containing either one number or a triad separated by commas'
        return issues

    #pylint: disable=too-many-locals, too-many-branches
    def PyExec(self):
        # Save current configuration
        facility = config['default.facility']
        instrument = config['default.instrument']
        # Allows searching for ARCS run numbers
        config['default.facility'] = 'SNS'
        config['default.instrument'] = 'ARCS'
        self._runs = self.getProperty('RunNumbers').value
        self._vanfile = self.getProperty('Vanadium').value
        self._ecruns = self.getProperty('EmptyCanRunNumbers').value
        self._ebins_str = self.getProperty('EnergyBins').value
        self._qbins_str = self.getProperty('MomentumTransferBins').value
        self._snorm = self.getProperty('NormalizeSlices').value
        self._clean = self.getProperty('CleanWorkspaces').value
        wn_sqes = self.getPropertyValue("OutputWorkspace")

        # workspace names
        prefix = ''
        if self._clean:
            prefix = '__'
        # Sample files
        wn_data = prefix + 'data'
        wn_van = prefix + 'vanadium'
        wn_reduced = prefix + 'reduced'
        wn_ste = prefix + 'S_theta_E'
        wn_van_st = prefix + 'vanadium_S_theta'
        wn_sten = prefix + 'S_theta_E_normalized'
        wn_steni = prefix + 'S_theta_E_normalized_interp'
        wn_sqe = prefix + 'S_Q_E'
        wn_sqeb = prefix + 'S_Q_E_binned'
        wn_sqesn = prefix + wn_sqes + '_norm'
        # Empty can files
        wn_ec_data = prefix + 'ec_data'
        wn_ec_reduced = prefix + 'ec_reduced'
        wn_ec_ste = prefix + 'ec_S_theta_E'

        datasearch = config["datasearch.searcharchive"]
        if datasearch != "On":
            config["datasearch.searcharchive"] = "On"

        try:
            # Load several event files into a sinle workspace. The nominal incident
            # energy should be the same to avoid difference in energy resolution
            api.Load(Filename=self._runs, OutputWorkspace=wn_data)

            # Load the vanadium file, assume to be preprocessed, meaning that
            # for every detector all events whithin a particular wide wavelength
            # range have been rebinned into a single histogram
            api.Load(Filename=self._vanfile, OutputWorkspace=wn_van)

            # Load empty can event files, if present
            if self._ecruns:
                api.Load(Filename=self._ecruns, OutputWorkspace=wn_ec_data)
        finally:
            # Recover the default configuration
            config['default.facility'] = facility
            config['default.instrument'] = instrument
            config["datasearch.searcharchive"] = datasearch

        # Retrieve the mask from the vanadium workspace, and apply it to the data
        # (and empty can, if submitted)
        api.MaskDetectors(Workspace=wn_data, MaskedWorkspace=wn_van)
        if self._ecruns:
            api.MaskDetectors(Workspace=wn_ec_data, MaskedWorkspace=wn_van)

        # Obtain incident energy as the mean of the nominal Ei values.
        # There is one nominal value per events file.
        ws_data = api.mtd[wn_data]
        Ei = ws_data.getRun()['EnergyRequest'].getStatistics().mean
        Ei_std = ws_data.getRun()['EnergyRequest'].getStatistics().standard_deviation

        # Verify empty can runs were obtained at similar energy
        if self._ecruns:
            ws_ec_data = api.mtd[wn_ec_data]
            ec_Ei = ws_ec_data.getRun()['EnergyRequest'].getStatistics().mean
            if abs(Ei - ec_Ei) > Ei_std:
                raise RuntimeError('Empty can runs were obtained at a significant' +
                                   ' different incident energy than the sample runs')

        # Obtain energy range
        self._ebins = [float(x) for x in re.compile(r'\d+[\.\d+]*').findall(self._ebins_str)]
        if len(self._ebins) == 1:
            ws_data = api.mtd[wn_data]
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
        factor = 0.1  # a fine energy bin
        Erange = '{0},{1},{2}'.format(self._ebins[0], factor * self._ebins[1], self._ebins[2])
        api.DgsReduction(SampleInputWorkspace=wn_data,
                         EnergyTransferRange=Erange, OutputWorkspace=wn_reduced)
        if self._ecruns:
            api.DgsReduction(SampleInputWorkspace=wn_ec_data,
                             EnergyTransferRange=Erange, IncidentBeamNormalisation='ByCurrent',
                             OutputWorkspace=wn_ec_reduced)

        # Obtain maximum and minimum |Q| values, as well as dQ if none passed
        self._qbins = [float(x) for x in re.compile(r'\d+[\.\d+]*').findall(self._qbins_str)]
        if len(self._qbins) < 3:
            if not self._qbins:
                # insert dQ if empty qbins
                dE = self._ebins[1]
                self._qbins.append(numpy.sqrt((Ei + dE) / ENERGY_TO_WAVEVECTOR) -
                                   numpy.sqrt(Ei / ENERGY_TO_WAVEVECTOR))
            mins, maxs = api.ConvertToMDMinMaxLocal(wn_reduced, Qdimensions='|Q|',
                                                    dEAnalysisMode='Direct')
            self._qbins.insert(0, mins[0])  # prepend minimum Q
            self._qbins.append(maxs[0])  # append maximum Q

        # Clean up the events files. They take a lot of space in memory
        api.DeleteWorkspace(wn_data)
        if self._ecruns:
            api.DeleteWorkspace(wn_ec_data)

        # Convert to S(theta,E)
        ki = numpy.sqrt(Ei / ENERGY_TO_WAVEVECTOR)
        factor = 1. / 5  # a reasonable (heuristic) value
        # If dE is the smallest energy transfer considered,
        # then dQ/ki is the smallest dtheta (in radians)
        dtheta = factor * self._qbins[1] / ki * (180.0 / numpy.pi)
        # very small dtheta (<0.15 degrees) prevents interpolation
        dtheta = max(0.15, dtheta)
        group_file_os_handle, group_file_name = mkstemp(suffix='.xml')
        group_file_handle = os.fdopen(group_file_os_handle, 'w')
        api.GenerateGroupingPowder(InputWorkspace=wn_reduced, AngleStep=dtheta,
                                   GroupingFilename=group_file_name)
        group_file_handle.close()
        api.GroupDetectors(InputWorkspace=wn_reduced, MapFile=group_file_name,
                           OutputWorkspace=wn_ste)
        if self._ecruns:
            api.GroupDetectors(InputWorkspace=wn_ec_reduced, MapFile=group_file_name,
                               OutputWorkspace=wn_ec_ste)
            # Substract the empty can from the can+sample
            api.Minus(LHSWorkspace=wn_ste, RHSWorkspace=wn_ec_ste, OutputWorkspace=wn_ste)

        # Normalize by the vanadium intensity, but before that we need S(theta)
        # for the vanadium. Recall every detector has all energies into a single
        # bin, so we get S(theta) instead of S(theta,E)
        api.GroupDetectors(InputWorkspace=wn_van, MapFile=group_file_name,
                           OutputWorkspace=wn_van_st)
        os.remove(group_file_name)  # no need for this file
        api.Divide(wn_ste, wn_van_st, OutputWorkspace=wn_sten)
        api.ClearMaskFlag(Workspace=wn_sten)

        max_i_theta = 0.0
        min_i_theta = 0.0

        # Linear interpolation
        # First, find minimum theta index with a non-zero histogram
        ws_sten = api.mtd[wn_sten]
        for i_theta in range(ws_sten.getNumberHistograms()):
            if ws_sten.dataY(i_theta).any():
                min_i_theta = i_theta
                break
        # second, find maximum theta with a non-zero histogram
        for i_theta in range(ws_sten.getNumberHistograms() - 1, -1, -1):
            if ws_sten.dataY(i_theta).any():
                max_i_theta = i_theta
                break
        # Scan the region [min_i_theta, max_i_theta] and apply interpolation to
        # theta angles with no signal whatsoever, S(theta*, E)=0.0 for all energies
        api.CloneWorkspace(InputWorkspace=wn_sten, OutputWorkspace=wn_steni)
        ws_steni = api.mtd[wn_steni]
        i_theta = 1 + min_i_theta
        while i_theta < max_i_theta:
            if not ws_steni.dataY(i_theta).any():
                nonnull_i_theta_start = i_theta - 1  # angle index of non-null histogram
                # scan until we find a non-null histogram
                while not ws_steni.dataY(i_theta).any():
                    i_theta += 1
                nonnull_i_theta_end = i_theta  # angle index of non-null histogram
                # The range [1+nonnull_i_theta_start, nonnull_i_theta_end]
                # contains only null-histograms. Interpolate!
                y_start = ws_steni.dataY(nonnull_i_theta_start)
                y_end = ws_steni.dataY(nonnull_i_theta_end)
                intercept = y_start
                slope = (y_end - y_start) / (nonnull_i_theta_end - nonnull_i_theta_start)
                for null_i_theta in range(1 + nonnull_i_theta_start, nonnull_i_theta_end):
                    ws_steni.dataY(null_i_theta)[:] = intercept + slope * (null_i_theta - nonnull_i_theta_start)
            i_theta += 1

        # Convert S(theta,E) to S(Q,E), then rebin in |Q| and E to MD workspace
        api.ConvertToMD(InputWorkspace=wn_steni, QDimensions='|Q|',
                        dEAnalysisMode='Direct', OutputWorkspace=wn_sqe)
        Qmin = self._qbins[0]
        Qmax = self._qbins[-1]
        dQ = self._qbins[1]
        Qrange = '|Q|,{0},{1},{2}'.format(Qmin, Qmax, int((Qmax - Qmin) / dQ))
        Ei_min = self._ebins[0]
        Ei_max = self._ebins[-1]
        dE = self._ebins[1]
        deltaErange = 'DeltaE,{0},{1},{2}'.format(Ei_min, Ei_max, int((Ei_max - Ei_min) / dE))
        api.BinMD(InputWorkspace=wn_sqe, AxisAligned=1, AlignedDim0=Qrange,
                  AlignedDim1=deltaErange, OutputWorkspace=wn_sqeb)

        # Slice the data by transforming to a Matrix2Dworkspace, with deltaE along the vertical axis
        api.ConvertMDHistoToMatrixWorkspace(InputWorkspace=wn_sqeb,
                                            Normalization='NumEventsNormalization', OutputWorkspace=wn_sqes)

        # Ensure correct units
        api.mtd[wn_sqes].getAxis(0).setUnit("MomentumTransfer")
        api.mtd[wn_sqes].getAxis(1).setUnit("DeltaE")

        # Shift the energy axis, since the reported values should be the center
        # of the bins, instead of the minimum bin boundary
        ws_sqes = api.mtd[wn_sqes]
        Eaxis = ws_sqes.getAxis(1)
        e_shift = self._ebins[1] / 2.0
        for i in range(Eaxis.length()):
            Eaxis.setValue(i, Eaxis.getValue(i) + e_shift)

        # Normalize each slice
        if self._snorm:
            api.Integration(InputWorkspace=wn_sqes, OutputWorkspace=wn_sqesn)
            api.Divide(LHSWorkspace=wn_sqes, RHSWorkspace=wn_sqesn, OutputWorkspace=wn_sqes)

        # Clean up workspaces from intermediate steps
        if self._clean:
            for name in (wn_van, wn_reduced, wn_ste, wn_van_st, wn_sten,
                         wn_steni, wn_sqe, wn_sqeb, wn_sqesn, 'PreprocessedDetectorsWS'):
                if api.mtd.doesExist(name):
                    api.DeleteWorkspace(name)

        # Ouput some info
        message = '\n******  SOME OUTPUT INFORMATION ***' + \
                  '\nEnergy bins: ' + ', '.join(['{0:.2f}'.format(x) for x in self._ebins]) + \
                  '\nQ bins: ' + ', '.join(['{0:.2f}'.format(x) for x in self._qbins]) + \
                  '\nTheta bins: {0:.2f} {1:.2f} {2:.2f}'.format(min_i_theta * dtheta, dtheta, max_i_theta * dtheta)
        logger.notice(message)

        self.setProperty("OutputWorkspace", api.mtd[wn_sqes])


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(DPDFreduction)
