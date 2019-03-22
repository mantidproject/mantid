# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from collections import namedtuple
from contextlib import contextmanager
from enum import Enum

from mantid import config as mantid_config
from mantid.api import (AnalysisDataService, DataProcessorAlgorithm,
                        AlgorithmFactory, FileProperty, FileAction,
                        WorkspaceProperty, PropertyMode)
from mantid.simpleapi import (DeleteWorkspace, LoadMask, LoadEventNexus,
                              CloneWorkspace, MaskDetectors,
                              ModeratorTzeroLinear, ConvertUnits,
                              CropWorkspace, RenameWorkspace,
                              LoadNexusMonitors, OneMinusExponentialCor,
                              Scale, Divide, Rebin, MedianDetectorTest,
                              SumSpectra, Integration, CreateWorkspace,
                              ScaleX, Plus)
from mantid.kernel import (FloatArrayProperty, Direction, logger)
debug_flag = False  # set to True to prevent erasing temporary workspaces


class VDAS(Enum):
    """Specifices the version of the Data Acquisition System (DAS)"""
    v1900_2018 = 0  # Up to Dec 31 2018
    v2019_2100 = 1  # From Jan 01 2018


@contextmanager
def pyexec_setup(new_options):
    """
    Backup keys of mantid.config and clean up temporary files and workspaces
    upon algorithm completion or exception raised.
    Workspaces with names beginning with '_t_' are assumed temporary.

    Parameters
    ----------
    new_options: dict
        Dictionary of mantid configuration options to be modified.
    """
    # Hold in this tuple all temporary objects to be removed after completion
    temp_objects = namedtuple('temp_objects', 'files workspaces')
    temps = temp_objects(list(), list())

    previous_config = dict()
    for key, value in new_options.items():
        previous_config[key] = mantid_config[key]
        mantid_config[key] = value
    try:
        yield temps
    finally:
        # reinstate the mantid options
        for key, value in previous_config.items():
            mantid_config[key] = value
        if debug_flag is True:
            return
        # delete temporary files
        for file_name in temps.files:
            os.remove(file_name)
        # remove any workspace added to temps.workspaces or whose name begins
        # with "_t_"
        to_be_removed = set()
        for name in AnalysisDataService.getObjectNames():
            if '_t_' == name[0:3]:
                to_be_removed.add(name)
        for workspace in temps.workspaces:
            if isinstance(workspace, str):
                to_be_removed.add(workspace)
            else:
                to_be_removed.add(workspace.name())
        for name in to_be_removed:
            DeleteWorkspace(name)


class BASISPowderDiffraction(DataProcessorAlgorithm):

    _mask_file = '/SNS/BSS/shared/autoreduce/new_masks_08_12_2015/'\
                 'BASIS_Mask_default_diff.xml'
    # Consider only events with these wavelengths
    _wavelength_bands = {'311': [3.07, 3.6], '111': [6.1, 6.6]}
    _diff_bank_numbers = list(range(5, 14))
    _tzero = dict(gradient=11.967, intercept=-5.0)

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)
        self._wavelength_band = None
        self._wavelength_dl = 0.0025  # in Angstroms
        self._qbins = None
        self._short_inst = "BSS"
        self._run_list = None
        self._temps = None
        self._bkg = None  # Events workspace for brackground runs
        self._bkg_scale = None
        self._van = None  # workspace for vanadium files
        self._v_mask = None  # mask pixels with low-counts in vanadium runs
        self._t_mask = None  # mask workspace
        self._das_version = None  # version of the Data Acquisition System

    @staticmethod
    def category():
        return "Diffraction\\Reduction"

    @staticmethod
    def version():
        return 1

    @staticmethod
    def summary():
        return 'Diffraction pattern for powder samples'

    @staticmethod
    def seeAlso():
        return ['BASISReduction', 'BASISCrystalDiffraction']

    @staticmethod
    def add_previous_pulse(w):
        """
        Duplicate the events but shift them by one pulse, then add to
        input workspace

        Parameters
        ----------
        w: Mantid.EventsWorkspace

        Returns
        -------
        Mantid.EventsWorkspace
        """
        pulse_width = 1.e6/60  # in micro-seconds
        _t_w = ScaleX(w, Factor=-pulse_width, Operation='Add')
        _t_w = Plus(w, _t_w, OutputWorkspace=w.name())
        return _t_w

    def PyInit(self):
        #
        # Properties
        #
        self.declareProperty('RunNumbers', '', 'Sample run numbers')

        self.declareProperty(FloatArrayProperty('MomentumTransferBins',
                                                [0.1, 0.0025, 2.5],  # invers A
                                                direction=Direction.Input),
                             'Momentum transfer binning scheme')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             doc='Output reduced workspace')
        #
        # Common Properties
        #
        required_title = 'Required Properties'

        self.declareProperty(FileProperty(name='MaskFile',
                                          defaultValue=self._mask_file,
                                          action=FileAction.OptionalLoad,
                                          extensions=['.xml']),
                             doc='See documentation for latest mask files.')

        self.declareProperty('MonitorNormalization', True,
                             'Normalization with wavelength-dependent '
                             'monitor counts')
        for a_property in ('MaskFile', 'MonitorNormalization'):
            self.setPropertyGroup(a_property, required_title)
        #
        # Background for the sample runs
        #
        background_title = 'Background runs'
        self.declareProperty('BackgroundRuns', '', 'Background run numbers')
        self.setPropertyGroup('BackgroundRuns', background_title)
        self.declareProperty("BackgroundScale", 1.0,
                             doc='The background will be scaled by this ' +
                                 'number before being subtracted.')
        self.setPropertyGroup('BackgroundScale', background_title)
        self.declareProperty(WorkspaceProperty('OutputBackground', '',
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             doc='Reduced workspace for background runs')
        self.setPropertyGroup('OutputBackground', background_title)
        #
        # Vanadium
        #
        vanadium_title = 'Vanadium runs'
        self.declareProperty('VanadiumRuns', '', 'Vanadium run numbers')
        self.setPropertyGroup('VanadiumRuns', vanadium_title)

    def PyExec(self):
        # Facility and database configuration
        config_new_options = {'default.facility': 'SNS',
                              'default.instrument': 'BASIS',
                              'datasearch.searcharchive': 'On'}
        #
        # Find desired Q-binning
        #
        self._qbins = np.array(self.getProperty('MomentumTransferBins').value)
        #
        # implement with ContextDecorator after python2 is deprecated)
        #
        with pyexec_setup(config_new_options) as self._temps:
            #
            # Load the mask to a temporary workspace
            #
            self._t_mask = LoadMask(Instrument='BASIS',
                                    InputFile=self.getProperty('MaskFile').
                                    value,
                                    OutputWorkspace='_t_mask')
            #
            # Find the version of the Data Acquisition System
            #
            self._find_das_version()
            #
            # Calculate the valid range of wavelengths for incoming neutrons
            #
            self._calculate_wavelength_band()
            #
            # Load and process vanadium runs, if applicable
            #
            if self.getProperty('VanadiumRuns').value != '':
                self._load_vanadium_runs()
            #
            # Process the sample
            #
            runs = self.getProperty('RunNumbers').value
            _t_sample = self._load_runs(runs, '_t_sample')
            _t_sample = self._apply_corrections_vanadium(_t_sample)
            if self.getProperty('BackgroundRuns').value != '':
                _t_sample, _t_bkg = self._subtract_background(_t_sample)
                if self.getPropertyValue('OutputBackground') != '':
                    _t_bkg_angle = self._convert_to_angle(_t_bkg)
                    self._output_workspace(_t_bkg_angle, 'OutputBackground',
                                           suffix='_angle')
                    _t_bkg = self._convert_to_q(_t_bkg)
                    self._output_workspace(_t_bkg, 'OutputBackground')
            _t_sample_angle = self._convert_to_angle(_t_sample)
            self._output_workspace(_t_sample_angle, 'OutputWorkspace',
                                   suffix='_angle')
            _t_sample = self._convert_to_q(_t_sample)
            self._output_workspace(_t_sample, 'OutputWorkspace')

    @staticmethod
    def _run_lists(runs):
        """
        Obtain all run numbers from input string `runs`

        Parameters
        ----------
        runs: str
            Run numbers to be reduced. Symbol `;` separates the runs into
            substrings. Each substring represents a set of runs to be
            reduced together.
        Returns
        -------

        """
        rl = list()
        rn = runs.replace(' ', '')  # remove spaces
        for x in rn.split(','):
            if '-' in x:
                b, e = [int(y) for y in x.split('-')]
                rl.extend([str(z) for z in range(b, e+1)])
            else:
                rl.append(x)
        return rl

    def _load_runs(self, runs, w_name):
        """
        Load all run event Nexus files into a single `EventWorkspace`

        Parameters
        ----------
        runs: str
            Run numbers to be reduced. Symbol `;` separates the runs into
            substrings. Each substring represents a set of runs to be
            reduced together
        w_name: str
            Name of output workspace

        Returns
        -------
        Mantid.EventsWorkspace
        """
        rl = self._run_lists(runs)
        #
        # Load files together
        #
        _t_all_w = None
        for run in rl:
            _t_w = self._load_single_run(run, '_t_w')
            if _t_all_w is None:
                _t_all_w = CloneWorkspace(_t_w)
            else:
                _t_all_w += _t_w
        RenameWorkspace(_t_all_w, OutputWorkspace=w_name)
        return _t_all_w

    def _apply_corrections_vanadium(self, w, target='sample'):
        """
        Apply a series of corrections and normalizations to the input
        workspace, plus normalization by vanadium.

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventsWorkspace
        """
        _t_corr_van = self._apply_corrections(w, target=target)
        if self.getProperty('VanadiumRuns').value != '':
            _t_corr_van = self._sensitivity_correction(_t_corr_van)
        RenameWorkspace(_t_corr_van, OutputWorkspace=w.name())
        return _t_corr_van

    def _apply_corrections(self, w, target='sample'):
        """
        Apply a series of corrections and normalizations to the input
        workspace

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventsWorkspace
        """
        MaskDetectors(w, MaskedWorkspace=self._t_mask)
        _t_corr = ModeratorTzeroLinear(w,
                                       Gradient=self._tzero['gradient'],
                                       Intercept=self._tzero['intercept'])
        # Correct old DAS shift of fast neutrons. See GitHub issue 23855
        if self._das_version == VDAS.v1900_2018:
            _t_corr = self.add_previous_pulse(_t_corr)
        _t_corr = ConvertUnits(_t_corr, Target='Wavelength', Emode='Elastic')
        l_s, l_e = self._wavelength_band[0], self._wavelength_band[1]
        _t_corr = CropWorkspace(_t_corr, XMin=l_s, XMax=l_e)
        _t_corr = Rebin(_t_corr, Params=[l_s, self._wavelength_dl, l_e],
                        PreserveEvents=False)
        if self.getProperty('MonitorNormalization').value is True:
            _t_corr = self._monitor_normalization(_t_corr, target)
        return _t_corr

    def _load_monitors(self, target):
        """
        Load monitor data for all target runs into a single workspace

        Parameters
        ----------
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventWorkspace
        """
        valid_targets = ('sample', 'background', 'vanadium')
        if target not in valid_targets:
            raise KeyError('Target must be one of ' + ', '.join(valid_targets))
        target_to_runs = dict(sample='RunNumbers', background='BackgroundRuns',
                              vanadium='VanadiumRuns')
        #
        # Load monitors files together
        #
        rl = self._run_lists(self.getProperty(target_to_runs[target]).value)
        _t_all_w = None
        for run in rl:
            file_name = "{0}_{1}_event.nxs".format(self._short_inst, str(run))
            _t_w = LoadNexusMonitors(file_name)
            if _t_all_w is None:
                _t_all_w = CloneWorkspace(_t_w)
            else:
                _t_all_w += _t_w
        return _t_all_w

    def _monitor_normalization(self, w, target):
        """
        Divide data by integrated monitor intensity

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventWorkspace
        """
        _t_mon = self._load_monitors(target)
        _t_mon = ConvertUnits(_t_mon, Target='Wavelength', Emode='Elastic')
        _t_mon = CropWorkspace(_t_mon, XMin=self._wavelength_band[0],
                               XMax=self._wavelength_band[1])
        _t_mon = OneMinusExponentialCor(_t_mon, C='0.20749999999999999',
                                        C1='0.001276')
        _t_mon = Scale(_t_mon, Factor='1e-06', Operation='Multiply')
        _t_mon = Integration(_t_mon)  # total monitor count
        _t_w = Divide(w, _t_mon, OutputWorkspace=w.name())
        return _t_w

    def _load_vanadium_runs(self):
        """
        Initialize the vanadium workspace and the related mask to avoid using
        pixels with low-counts.
        """
        runs = self.getProperty('VanadiumRuns').value
        _t_van = self._load_runs(runs, '_t_van')
        _t_van = self._apply_corrections(_t_van, target='vanadium')
        RenameWorkspace(_t_van, OutputWorkspace='_t_van')
        wave_band = self._wavelength_band[1] - self._wavelength_band[0]
        _t_van = Rebin(_t_van, Params=[self._wavelength_band[0], wave_band,
                                       self._wavelength_band[1]])
        output = MedianDetectorTest(_t_van, OutputWorkspace='_t_v_mask')
        self._v_mask = output.OutputWorkspace
        MaskDetectors(_t_van, MaskedWorkspace=self._v_mask)
        self._van = _t_van

    def _sensitivity_correction(self, w):
        """
        Divide each pixel by the vanadium count

        Parameters
        ----------
        w: Events workspace in units of wavelength
        Returns
        -------
        Mantid.EventWorkspace
        """
        MaskDetectors(w, MaskedWorkspace=self._v_mask)
        _t_w = Divide(w, self._van, OutputWorkspace=w.name())
        return _t_w

    def _subtract_background(self, w):
        """
        Subtract background from sample

        Parameters
        ----------
        w: Mantid.EventWorkspace
            Sample workspace from which to subtract the background

        Returns
        -------
        Mantid.EventWorkspace
        """
        runs = self.getProperty('BackgroundRuns').value
        _t_bkg = self._load_runs(runs, '_t_bkg')
        _t_bkg = self._apply_corrections_vanadium(_t_bkg, target='background')
        x = self.getProperty('BackgroundScale').value
        _t_w = w - x * _t_bkg
        RenameWorkspace(_t_w, OutputWorkspace=w.name())
        return _t_w, _t_bkg

    def _convert_to_q(self, w):
        """
        Convert to momentum transfer with the desired binning

        Parameters
        ----------
        w: Mantid.MatrixWorkspace2D

        Returns
        -------
        Mantid.MatrixWorkspace2D
        """
        _t_w = ConvertUnits(w, Target='MomentumTransfer', Emode='Elastic')
        _t_w = Rebin(_t_w, Params=self._qbins, PreserveEvents=False)
        _t_w = SumSpectra(_t_w, OutputWorkspace=w.name())
        return _t_w

    def _convert_to_angle(self, w):
        """
        Output the integrated intensity for each elastic detector versus
        detector angle with the neutron beam.

        Masked elastic detectors are assigned a zero intensity

        Parameters
        ----------
        w: Mantid.MatrixWorkspace2D

        Returns
        -------
        Mantid.MatrixWorkspace2D
        """
        id_s, id_e = 16386, 17534  # start and end for elastic detector ID's
        _t_w = Integration(w)
        sp = _t_w.spectrumInfo()
        x, y, e = [list(), list(), list()]
        for i in range(_t_w.getNumberHistograms()):
            id_i = _t_w.getDetector(i).getID()
            if id_s <= id_i <= id_e:
                x.append(np.degrees(sp.twoTheta(i)))
                if sp.isMasked(i) is True:
                    y.append(0.0)
                    e.append(1.0)
                else:
                    y.append(_t_w.readY(i)[0])
                    e.append(_t_w.readE(i)[0])
        x = np.asarray(x)
        y = np.asarray(y)
        e = np.asarray(e)
        od = np.argsort(x)  # order in ascending angles
        title = 'Angle between detector and incoming neutron beam'
        _t_w = CreateWorkspace(DataX=x[od], DataY=y[od], DataE=e[od],
                               NSpec=1, UnitX='Degrees',
                               WorkspaceTitle=title)
        return _t_w

    def _output_workspace(self, w, prop, suffix=''):
        """
        Rename workspace and set the related output property

        Parameters
        ----------
        w: Mantid.MatrixWorkspace
        prop: str
            Output property name
        """
        w_name = self.getProperty(prop).valueAsStr + suffix
        RenameWorkspace(w, OutputWorkspace=w_name)
        self.setProperty(prop, w)

    def _find_das_version(self):
        boundary_run = 90000  # from VDAS.v1900_2018 to VDAS.v2019_2100
        runs = self.getProperty('RunNumbers').value
        first_run = int(self._run_lists(runs)[0])
        if first_run < boundary_run:
            self._das_version = VDAS.v1900_2018
        else:
            self._das_version = VDAS.v2019_2100
        logger.information('DAS version is ' + str(self._das_version))

    def _calculate_wavelength_band(self):
        """
        Select the wavelength band examining the logs of the first sample
        """
        runs = self.getProperty('RunNumbers').value
        run = self._run_lists(runs)[0]
        _t_w = self._load_single_run(run, '_t_w')
        wavelength = np.mean(_t_w.getRun().getProperty('LambdaRequest').value)
        wavs = self._wavelength_bands
        midpoint = (wavs['111'][0] + wavs['311'][0]) / 2.0
        reflection = '111' if wavelength > midpoint else '311'
        self._wavelength_band = self._wavelength_bands[reflection]

    def _load_single_run(self, run, name):
        """
        Find and load events from the diffraction tubes.

        Run number 90000 discriminates between the old and new DAS

        Parameters
        ----------
        run: str
            Run number
        name: str
            Name of the output EventsWorkspace

        Returns
        -------
        EventsWorkspace
        """
        banks = ','.join(['bank{}'.format(i) for i in self._diff_bank_numbers])
        particular = {VDAS.v1900_2018: dict(NXentryName='entry-diff'),
                      VDAS.v2019_2100: dict(BankName=banks)}
        file_name = "{0}_{1}_event.nxs".format(self._short_inst, str(run))
        kwargs = dict(Filename=file_name, SingleBankPixelsOnly=False,
                      OutputWorkspace=name)
        kwargs.update(particular[self._das_version])
        return LoadEventNexus(**kwargs)

 
# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISPowderDiffraction)
