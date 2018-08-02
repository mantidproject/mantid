from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid  # Have to import Mantid to setup paths
import unittest
import tempfile

from isis_powder.gem_routines import gem_output


class ISISPowderGemOutputTest(unittest.TestCase):

    IPF_FILE = "GEM_PF1_PROFILE.IPF"

    def _find_file_or_die(self, name):
        full_path = mantid.api.FileFinder.getFullPath(name)
        if not full_path:
            self.fail("Could not find file \"{}\"".format(name))
        return full_path

    def _get_temp_filename(self):
        return








    def test_ipf(self):
        pathToIpf = self._find_file_or_die(IPF_FILE)
        fileName = self._get_temp_filename()

