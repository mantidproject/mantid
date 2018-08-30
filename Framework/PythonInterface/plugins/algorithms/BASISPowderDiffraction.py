from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from collections import namedtuple
from contextlib import contextmanager

from mantid import config as mantid_config
from mantid.api import (AnalysisDataService, DataProcessorAlgorithm,
                        AlgorithmFactory, FileProperty, FileAction,
                        WorkspaceProperty, PropertyMode)
from mantid.simpleapi import (DeleteWorkspace)
from mantid.kernel import (FloatArrayLengthValidator, FloatArrayProperty,
                           Direction)

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
    _solid_angle_ws_ = '/SNS/BSS/shared/autoreduce/solid_angle_diff.nxs'
    _flux_ws_ = '/SNS/BSS/shared/autoreduce/int_flux.nxs'

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)
        self._lambda_range = [5.86, 6.75]  # units of inverse Angstroms
        self._default_qbins = [0.1, 0.05, 2.1]  # inverse Angstroms
        self._short_inst = "BSS"
        self._long_inst = "BASIS"
        self._run_list = None
        self._temps = None
        self._bkg = None  # Events workspace for brackground runs
        self._bkg_scale = None
        self._vanadium_files = None
        self._momentum_range = None
        self._t_mask = None

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
                                                self._default_qbins,
                                                direction=Direction.Input),
                             'Momentum transfer binning scheme')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             doc='Output Workspace. If background is '+
                                 'subtracted, _data and _background '+
                                 'workspaces will also be generated')
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
                                                self._lambda_range,
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
        #
        # Vanadium
        #
        vanadium_title = 'Vanadium runs'
        self.declareProperty('VanadiumRuns', '', 'Vanadium run numbers')
        self.setPropertyGroup('VanadiumRuns', vanadium_title)


    def PyExec(self):
        pass

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISPowderDiffraction)
