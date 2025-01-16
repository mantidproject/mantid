# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import FileFinder
import mantid.simpleapi as mantid  # Have to import Mantid to setup paths
import unittest
import tempfile
import shutil
import warnings

from isis_powder.gem_routines import gem_output


class ISISPowderGemOutputTest(unittest.TestCase):
    _folders_to_remove = set()
    CHECK_AGAINST = "BANK 1 100  25 RALF  -12266  96  -12266 -0.022 ALT                              \n"
    GROUPING_SCHEME = [1, 2]
    IPF_FILE = "GEM_PF1_PROFILE.IPF"

    def _find_file_or_die(self, name):
        full_path = FileFinder.getFullPath(name)
        if not full_path:
            self.fail('Could not find file "{}"'.format(name))
        return full_path

    def tearDown(self):
        for folder in self._folders_to_remove:
            try:
                shutil.rmtree(folder)
            except OSError as exc:
                warnings.warn('Could not remove folder at "{}"\nError message:\n{}'.format(folder, exc))

    def test_valid_save(self):
        path_to_ipf = self._find_file_or_die(self.IPF_FILE)
        temp_file_path = tempfile.mkdtemp()
        self._folders_to_remove.add(temp_file_path)
        wsgroup = []
        for i in range(2):
            wsgroup.append(
                mantid.CreateSampleWorkspace(
                    OutputWorkspace=str(i),
                    NumBanks=1,
                    BankPixelWidth=1,
                    XMin=-0.5,
                    XMax=0.5,
                    BinWidth=0.01,
                    XUnit="DSpacing",
                    StoreInADS=True,
                )
            )

        gem_output_test_ws_group = mantid.GroupWorkspaces(wsgroup)
        gem_output.save_gda(
            d_spacing_group=gem_output_test_ws_group,
            gsas_calib_filename=path_to_ipf,
            grouping_scheme=self.GROUPING_SCHEME,
            output_path=temp_file_path + "\\test.gda",
            raise_warning=False,
        )

        self._find_file_or_die(temp_file_path + "\\test.gda")

        with open(temp_file_path + "\\test.gda") as read_file:
            first_line = read_file.readline()
            self.assertEqual(first_line, self.CHECK_AGAINST)


if __name__ == "__main__":
    unittest.main()
