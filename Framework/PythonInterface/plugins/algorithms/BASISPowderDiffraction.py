from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from collections import namedtuple
from contextlib import contextmanager

from mantid import config as mantid_config
from mantid.api import (AnalysisDataService, DataProcessorAlgorithm,
                        AlgorithmFactory, FileProperty, FileAction,
                        WorkspaceProperty, PropertyMode)
from mantid.simpleapi import (DeleteWorkspace, LoadMask, LoadEventNexus,
                              CloneWorkspace, MaskDetectors,
                              ModeratorTzeroLinear, ConvertUnits,
                              CropWorkspace, RenameWorkspace,
                              LoadNexusMonitors, OneMinusExponentialCor,
                              Scale, RebinToWorkspace, Divide, Rebin,
                              MedianDetectorTest)
from mantid.kernel import (FloatArrayLengthValidator, FloatArrayProperty,
                           Direction, IntArrayProperty)

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

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)
        self._wavelength_band = None
        self._wavelength_nbins = 20
        self._qbins = None
        self._short_inst = "BSS"
        self._run_list = None
        self._temps = None
        self._bkg = None  # Events workspace for brackground runs
        self._bkg_scale = None
        self._van = None  # workspace for vanadium files
        self._v_mask = None  # mask pixels with low-counts in vanadium runs
        self._t_mask = None  # mask workspace

    @staticmethod
    def category():
        return "Diffraction\\Reduction"

    @staticmethod
    def version():
        return 1

    @staticmethod
    def summary():
        return 'Multiple-file BASIS powder reduction for diffraction ' \
               'detectors.'

    @staticmethod
    def seeAlso():
        return ['BASISReduction', 'BASISCrystalDiffraction']

    def PyInit(self):
        # Input validators
        array_length_three = FloatArrayLengthValidator(3)

        # Properties
        self.declareProperty('RunNumbers', '', 'Sample run numbers')

        self.declareProperty(FloatArrayProperty('MomentumTransferBins',
                                                [0.1, 0.05, 2.1],  # inverse A
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

        self.declareProperty(FloatArrayProperty('LambdaRange',
                                                [5.86, 6.75],  # inverse Angs
                                                direction=Direction.Input),
                             doc='Incoming neutron wavelength range')

        self.declareProperty('MonitorNormalization', True,
                             'Normalization with wavelength-dependent '
                             'monitor counts')
        for a_property in ('MaskFile', 'LambdaRange', 'MonitorNormalization'):
            self.setPropertyGroup(a_property, required_title)
        #
        # Background for the sample runs
        #
        background_title = 'Background runs'
        self.declareProperty('BackgroundRuns', '', 'Background run numbers')
        self.setPropertyGroup('BackgroundRuns', background_title)
        self.declareProperty("BackgroundScale", 1.0,
                             doc='The background will be scaled by this '+
                                 'number before being subtracted.')
        self.setPropertyGroup('BackgroundScale', background_title)
        self.declareProperty(WorkspaceProperty('OutputBackground', '',
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             doc='Reduced workspace for background runs')
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
        # Load the mask to a temporary workspace
        #
        self._t_mask = LoadMask(Instrument='BASIS',
                                InputFile=self.getProperty('MaskFile').
                                value,
                                OutputWorkspace='_t_mask')
        #
        # Desirece wavelength band of incoming neutrons
        #
        self._wavelength_band = np.array(self.getProperty('LambdaRange').value)
        #
        # Load and process vanadium runs, if applicable
        #
        if self.getProperty('VanadiumRuns').value != '':
            self._load_vanadium_runs()
        #
        # implement with ContextDecorator after python2 is deprecated)
        #
        with pyexec_setup(config_new_options) as self._temps:
            runs = self.getProperty('RunNumbers').value
            _t_sample = self._load_runs(runs, '_t_sample')
            self._apply_standard_corrections(_t_sample)
            if self.getProperty('BackgroundRuns').value != '':
                _t_bkg = self._substract_background(_t_sample)
                if self.getPropertyValue('OutputBackground') != '':
                    self._convert_to_q(_t_bkg)
                    self._output_workspace(_t_bkg, 'OutputBackground')
            self._convert_to_q(_t_sample)
            self._output_workspace(_t_bkg, 'OutputWorkspace')

    def _run_lists(self, runs, do_indiv=False):
        """
        Obtain all run numbers from input string `runs`

        Parameters
        ----------
        runs: str
            Run numbers to be reduced. Symbol `;` separates the runs into
            substrings. Each substring represents a set of runs to be
            reduced together
        do_indiv: bool
            Reduce each run number separately
        Returns
        -------

        """
        rl = list()
        rlvals = runs.split(';')
        for rlval in rlvals:
            iap = IntArrayProperty("", rlval)  # split the substring
            if do_indiv:
                raise NotImplementedError(
                    "Individual reduction not implemented")
                rl.extend([[x] for x in iap.value])
            else:
                rl.append(iap.value)
        return rl

    def _load_runs(self, runs, w_name, do_indiv=False):
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
        do_indiv: bool
            Reduce each run number separately

        Returns
        -------
        Mantid.EventsWorkspace
        """
        rl = self._run_lists(runs, do_indiv=do_indiv)
        #
        # Load files together
        #
        _t_all_w = None
        for run in rl:
            file_name = "{0}_{1}_event.nxs".format(self._short_inst, str(run))
            _t_w = LoadEventNexus(Filename=file_name,NXentryName='entry-diff',
                                  SingleBankPixelsOnly=False)
            if _t_all_w is None:
                _t_all_w = CloneWorkspace(_t_w)
            else:
                _t_all_w += _t_w
        RenameWorkspace(_t_all_w, OutputWorkspace=w_name)

    def _apply_standard_corrections(self, w, target='sample'):
        """
        Apply a series of corrections and normalizations to the input
        workspace

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample' and 'background'
        """
        _t_w = MaskDetectors(w, MaskedWorkspace=self._t_mask)
        _t_w = ModeratorTzeroLinear(_t_w)  # delayed emission from moderator
        _t_w = ConvertUnits(_t_w, Target='Wavelength', Emode='Elastic')
        _t_w = CropWorkspace(_t_w,
                             XMin=self._wavelength_band[0],
                             XMax=self._wavelength_band[1])
        _t_w = Rebin(_t_w, Params=str(self._wavelength_nbins))
        if self.getProperty('MonitorNorm').value is True:
            self._monitor_normalization(_t_w, target)
        if self.getProperty('VanadiumRuns').value != '':
            self._sensitivity_correction(_t_w)
        RenameWorkspace(_t_w, OutputWorkspace=w.name())

    def _load_monitors(self, target):
        """
        Load monitor data for all target runs into a single workspace

        Parameters
        ----------
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample' and 'background'

        Returns
        -------
        Mantid.EventWorkspace
        """
        valid_targets = ('sample', 'background')
        if target not in valid_targets:
            raise KeyError('Target must be one of ' + ', '.join(valid_targets))
        target_to_runs = dict(sample='RunNumbers', background='BackgroundRuns')
        #
        # Load monitors files together
        #
        rl = self._run_lists(target_to_runs[target], do_indiv=False)
        t_all_w = None
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
        Divide data by wavelength-dependent flux

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample' and 'background'
        """
        _t_w = self._load_monitors(target)
        _t_w = ConvertUnits(_t_w, Target='Wavelength', Emode='Elastic')
        _t_w = CropWorkspace(_t_w,
                             XMin=self._wavelength_band[0],
                             XMax=self._wavelength_band[1])
        _t_w = OneMinusExponentialCor(C='0.20749999999999999',
                                      C1='0.001276')
        _t_w = Scale(_t_w, Factor='1e-06')
        _t_w = RebinToWorkspace(_t_w, w)
        Divide(w, _t_w, OutputWorkspace=w.name())

    def _load_vanadium_runs(self):
        """
        Initialize the vanadium workspace and the related mask to avoid using
        pixels with low-counts.
        """
        runs = self.getProperty('VanadiumRuns').value
        _t_w = self._load_runs(runs, '_t_w')
        _t_w = MaskDetectors(_t_w, MaskedWorkspace=self._t_mask)
        _t_w = ModeratorTzeroLinear(_t_w)
        _t_w = ConvertUnits(_t_w, Target='Wavelength', Emode='Elastic')
        _t_w = CropWorkspace(_t_w,
                             XMin=self._wavelength_band[0],
                             XMax=self._wavelength_band[1])
        wave_band = self._wavelength_band[1] - self._wavelength_band[0]
        self._t_w = Rebin(_t_w, Params=[self._wavelength_band[0], wave_band,
                                        self._wavelength_band[1]])
        self._v_mask = MedianDetectorTest(_t_w, OutputWorkspace='_t_v_mask')
        self._van = MaskDetectors(_t_w, MaskedWorkspace=self._v_mask,
                                  OutputWorkspace='_t_van')

    def _sensitivity_correction(self, w):
        """
        Divide each pixel by the vanadium count

        Parameters
        ----------
        w: Events workspace in units of wavelength
        """
        _t_w = MaskDetectors(w, MaskedWorkspace=self._v_mask)
        Divide(_t_w, self._van, OutputWorkspace=w.name())

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
        self._apply_standard_corrections(_t_bkg, target='background')
        x = self.getProperty('BackgroundScale').value
        _t_w = w - x * _t_bkg
        RenameWorkspace(_t_w, OutputWorkspace=w.name())
        return _t_bkg

    def _convert_to_q(self, w):
        """
        Convert to momentum transfer with the desired binning

        Parameters
        ----------
        w: Mantid.EventWorkspace

        Returns
        -------
        Mantid.MatrixWorkspace
        """
        _t_w = ConvertUnits(w, Target='MomentumTransfer', Emode='Elastic')
        _t_w = Rebin(_t_w, Params=self._qbins, PreserveEvents=False,
                     OutputWorkspace=w.name())

    def output_workspace(self, w, prop):
        """
        Rename workspace and set the related output property

        Parameters
        ----------
        w: Mantid.MatrixWorkspace
        prop: str
            Output property name
        """
        w_name = self.getProperty(prop).value
        RenameWorkspace(w, OutputWorkspace=w_name)
        self.setProperty(prop, w)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISPowderDiffraction)
