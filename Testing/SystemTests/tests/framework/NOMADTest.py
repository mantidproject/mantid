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
import os
import tempfile


class MedianDetectorTestTest(systemtesting.MantidSystemTest):

    def requiredFiles(self):
        return ['NOM_144974_SingleBin.nxs', 'NOMAD_mask_gen_config.yml', 'NOM_144974_mask.xml']

    def runTest(self):
        _, file_xml_mask = tempfile.mkstemp(suffix='.xml')
        _, file_txt_mask = tempfile.mkstemp(suffix='.txt')
        LoadNexusProcessed(Filename='NOM_144974_SingleBin.nxs', OutputWorkspace='NOM_144974')
        NOMADMedianDetectorTest(InputWorkspace='NOM_144974',
                                ConfigurationFile='NOMAD_mask_gen_config.yml',
                                SolidAngleNorm=False,
                                OutputMaskXML=file_xml_mask,
                                OutputMaskASCII=file_txt_mask)
        os.remove(file_txt_mask)
        for file_name, workspace_name in [(file_xml_mask, 'mask_test'), ('NOM_144974_mask.xml', 'mask_reference')]:
            LoadMask(Instrument='NOMAD',
                     InputFile=file_name,
                     RefWorkspace='NOM_144974',
                     OutputWorkspace=workspace_name)
        os.remove(file_xml_mask)
        success = CompareWorkspaces(Workspace1='mask_test', Workspace2='mask_reference', CheckMasking=True).Result
        self.assertTrue(success)
