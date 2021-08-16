# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
import systemtesting
from mantid.simpleapi import CompareWorkspaces, LoadMask, LoadNexusProcessed, NOMADMedianDetectorTest

# standard imports
import tempfile


class MedianDetectorTestTest(systemtesting.MantidSystemTest):

    def requiredFiles(self):
        return ['NOM_144974_SingleBin.nxs', 'NOMAD_mask_gen_config.yml', 'NOM_144974_mask.xml']

    def runTest(self):
        _, file_mask = tempfile.mkstemp(suffix='.xml')
        LoadNexusProcessed(Filename='NOM_144974_SingleBin.nxs', OutputWorkspace='NOM_144974')
        NOMADMedianDetectorTest(InputWorkspace='NOM_144974',
                                ConfigurationFile='NOMAD_mask_gen_config.yml',
                                OutputMaskXML=file_mask)
        for file_name, workspace_name in [(file_mask, 'mask_test'), ('NOM_144974_mask.xml', 'mask_reference')]:
            LoadMask(Instrument='NOMAD',
                     InputFile=file_mask,
                     RefWorkspace='NOM_144974',
                     OutputWorkspace=workspace_name)
        success = CompareWorkspaces(Workspace1='mask_test', Workspace2='mask_reference', CheckMasking=True).Result
        self.assertTrue(success)
