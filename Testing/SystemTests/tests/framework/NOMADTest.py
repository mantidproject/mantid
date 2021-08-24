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
        with tempfile.NamedTemporaryFile(suffix='.xml') as xml_handle, \
                tempfile.NamedTemporaryFile(suffix='.txt') as txt_handle:
            file_xml_mask = xml_handle.name
            file_txt_mask = txt_handle.name
            LoadNexusProcessed(Filename='NOM_144974_SingleBin.nxs', OutputWorkspace='NOM_144974')
            NOMADMedianDetectorTest(InputWorkspace='NOM_144974',
                                    ConfigurationFile='NOMAD_mask_gen_config.yml',
                                    SolidAngleNorm=False,
                                    OutputMaskXML=file_xml_mask,
                                    OutputMaskASCII=file_txt_mask)

            self.loaded_ws = LoadMask(Instrument='NOMAD',
                                      InputFile=file_xml_mask,
                                      RefWorkspace='NOM_144974',
                                      StoreInADS=False)

    def validate(self):
        ref = LoadMask(Instrument='NOMAD',
                       InputFile="NOM_144974_mask.xml",
                       RefWorkspace='NOM_144974',
                       StoreInADS=False)
        return CompareWorkspaces(Workspace1=self.loaded_ws, Workspace2=ref, CheckMasking=True).Result
