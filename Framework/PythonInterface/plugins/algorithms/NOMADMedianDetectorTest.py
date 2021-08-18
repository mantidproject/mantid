# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.api import (AlgorithmFactory, FileAction, FileProperty, MatrixWorkspace, PropertyMode, PythonAlgorithm,
                        WorkspaceProperty)
from mantid.kernel import Direction
from mantid.simpleapi import (DeleteWorkspaces, Divide, ExtractMask, LoadEmptyInstrument, MaskDetectors, mtd, SaveMask,
                              SolidAngle)

from mantid.utils.nomad.diagnostics import _NOMADMedianDetectorTest

# third-party imports
import numpy as np

# standard imports
import os


class NOMADMedianDetectorTest(PythonAlgorithm, _NOMADMedianDetectorTest):

    def category(self):
        """ Mantid required
        """
        return "Diagnostics"

    def seeAlso(self):
        return ["MedianDetectorTest"]

    def name(self):
        """ Mantid required
        """
        return "NOMADMedianDetectorTest"

    def summary(self):
        """ Mantid required
        """
        return "Identifies detector pixels and whole tubes having total numbers of counts over a user defined maximum" \
               " or less than a user define minimum."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty('InputWorkspace', defaultValue='', direction=Direction.Input,
                              optional=PropertyMode.Mandatory),
            doc='Workspace containing reference pixel intensities')

        self.declareProperty(
            FileProperty(name='ConfigurationFile', defaultValue='', direction=Direction.Input, extensions=['.yml'],
                         action=FileAction.Load),
            doc='YML file specifying collimation states and unused eight-packs')

        self.declareProperty(name='SolidAngleNorm', defaultValue=True, direction=Direction.Input,
                             doc='Normalize each pixel by its solid angle?')

        self.declareProperty(
            FileProperty('OutputMaskXML', defaultValue='', extensions=['.xml'],
                         action=FileAction.Save),
            doc='Output masked pixels in XML format')

        self.declareProperty(
            FileProperty('OutputMaskASCII', defaultValue='', extensions=['.txt'],
                         action=FileAction.Save),
            doc='Output masked pixels in single-column ASCII file')

    def PyExec(self):
        # initialize data structures 'config' and 'intensities'
        self.config = self.parse_yaml(self.getProperty('ConfigurationFile').value)
        self.intensities = self.get_intensities(self.getProperty('InputWorkspace').value,
                                                self.getProperty('SolidAngleNorm').value)
        # calculate the mask, and export it to XML and/or single-column ASCII file
        mask_composite = self.mask_by_tube_intensity | self.mask_by_pixel_intensity
        for exported in ['OutputMaskXML', 'OutputMaskASCII']:
            if self.getProperty(exported).value:
                self.export_mask(mask_composite, self.getProperty(exported).value)

    def get_intensities(self,
                        input_workspace: MatrixWorkspace,
                        solid_angle_normalize: bool = True
                        ) -> np.ma.core.MaskedArray:
        r"""
        Integrated intensity of each pixel for pixels in use. Pixels of unused eightpacks are masked

        Parameters
        ----------
        input_workspace: workspace containing pixel intensities
        solid_angle_normalize: carry out normalization by pixel solid angle

        Returns
        -------
        1D array of size number-of-pixels
        """
        intensities_workspace = mtd[str(input_workspace)]
        if solid_angle_normalize:
            solid_angles, normalized_workspace = self._random_string(), self._random_string()  # temporary workspaces
            SolidAngle(InputWorkspace=input_workspace, OutputWorkspace=solid_angles)
            Divide(LHSWorkspace=input_workspace, RHSWorkspace=solid_angles, OutputWorkspace=normalized_workspace)
            intensities_workspace = mtd[normalized_workspace]
        intensities = self._get_intensities(intensities_workspace)
        if solid_angle_normalize:
            DeleteWorkspaces([solid_angles, normalized_workspace])  # clean up temporary workspaces
        return intensities

    @classmethod
    def export_mask(cls,
                    pixel_mask_states: np.ndarray,
                    mask_file_name: str,
                    instrument_name: str = 'NOMAD') -> None:
        """
        Export masks to XML file format

        Parameters
        ----------
        pixel_mask_states: numpy.ndarray
            boolean array with the number of pixels of NOMAD.  True for masking
        mask_file_name: str
            name of the output mask XML file or single-colun ASCII file
        instrument_name: str
            name of the instrument
        """
        detector_ids_masked = np.where(pixel_mask_states)[0]  # detector ID's start at zero (monitors have negative ID)
        if '.xml' in mask_file_name:
            # Load empty instrument
            empty_workspace_name = cls._random_string()
            LoadEmptyInstrument(InstrumentName=instrument_name, OutputWorkspace=empty_workspace_name)

            # Get the workspace indexes to mask. Shift by the number of monitors
            if mtd[empty_workspace_name].getNumberHistograms() != pixel_mask_states.shape[0] + cls.MONITOR_COUNT:
                raise RuntimeError(f'Spectra number of {instrument_name} workspace does not match mask state array')
            mask_ws_indexes = detector_ids_masked + cls.MONITOR_COUNT

            MaskDetectors(Workspace=empty_workspace_name, WorkspaceIndexList=mask_ws_indexes)

            mask_workspace_name = cls._random_string()
            ExtractMask(InputWorkspace=empty_workspace_name, OutputWorkspace=mask_workspace_name)
            SaveMask(InputWorkspace=mask_workspace_name, OutputFile=mask_file_name)

            DeleteWorkspaces([empty_workspace_name, mask_workspace_name])
        else:
            np.savetxt(mask_file_name, detector_ids_masked, fmt='%6d', newline=os.linesep)


AlgorithmFactory.subscribe(NOMADMedianDetectorTest)
