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
from pathlib import Path


class MedianDetectorTestTest(systemtesting.MantidSystemTest):

    def requiredFiles(self):
        return ['NOM_144974_SingleBin.nxs', 'NOMAD_mask_gen_config.yml', 'NOM_144974_mask.xml']

    def runTest(self):
        # TemporaryFile will rather unhelpfully open a handle to our temporary object
        # On POSIX this is fine as "two" writers (Python and the save step) is a-okay
        # but on Windows this will throw. So instead create a temp directory and manage the lifetime
        with tempfile.TemporaryDirectory() as tmp_dir:
            self._test_impl(Path(tmp_dir))

    def _test_impl(self, tmp_dir: Path):
        file_xml_mask = (tmp_dir / "NOMADTEST.xml").resolve()
        file_txt_mask = (tmp_dir / "NOMADTEST.txt").resolve()
        LoadNexusProcessed(Filename='NOM_144974_SingleBin.nxs', OutputWorkspace='NOM_144974')
        NOMADMedianDetectorTest(InputWorkspace='NOM_144974',
                                ConfigurationFile='NOMAD_mask_gen_config.yml',
                                SolidAngleNorm=False,
                                OutputMaskXML=str(file_xml_mask),
                                OutputMaskASCII=str(file_txt_mask))

        self.loaded_ws = LoadMask(Instrument='NOMAD',
                                  InputFile=str(file_xml_mask),
                                  RefWorkspace='NOM_144974',
                                  StoreInADS=False)

    def validate(self):
        ref = LoadMask(Instrument='NOMAD',
                       InputFile="NOM_144974_mask.xml",
                       RefWorkspace='NOM_144974',
                       StoreInADS=False)
        return CompareWorkspaces(Workspace1=self.loaded_ws, Workspace2=ref, CheckMasking=True).Result
