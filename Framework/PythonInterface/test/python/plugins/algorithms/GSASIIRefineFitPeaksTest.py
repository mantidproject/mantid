from __future__ import (absolute_import, division, print_function)

import os
import unittest
from mantid.simpleapi import *
from mantid.api import *


class _GSASFinder(object):
    @staticmethod
    def _find_directory_by_name(cur_dir_name, cur_dir_path, name_to_find, level, max_level):
        if level == max_level:
            return None

        if cur_dir_name == name_to_find:
            return cur_dir_path

        list_dir = os.listdir(cur_dir_path)
        for child in list_dir:
            child_path = os.path.join(cur_dir_path, child)
            if os.path.isdir(child_path):
                try:
                    path = _GSASFinder._find_directory_by_name(cur_dir_name=child, cur_dir_path=child_path,
                                                               level=level + 1, name_to_find=name_to_find,
                                                               max_level=max_level)
                except WindowsError:
                    pass
                else:
                    if path is not None:
                        return path

    @staticmethod
    def _path_to_g2conda():
        root_directory = os.path.abspath(os.sep)
        return _GSASFinder._find_directory_by_name(cur_dir_path=root_directory, cur_dir_name=root_directory, level=0,
                                                   name_to_find="g2conda", max_level=5)

    @staticmethod
    def GSASIIscriptable_location():
        path_to_g2conda = _GSASFinder._path_to_g2conda()
        if path_to_g2conda is None:
            return None

        path_to_gsasii_scriptable = os.path.join(path_to_g2conda, "GSASII")
        if os.path.isfile(os.path.join(path_to_gsasii_scriptable, "GSASIIscriptable.py")):
            return path_to_gsasii_scriptable

        return None


class GSASIIRefineFitPeaksTest(unittest.TestCase):

    _path_to_gsas = None

    def setUp(self):
        self._path_to_gsas = _GSASFinder.GSASIIscriptable_location()
        if self._path_to_gsas is None:
            self.skipTest("Could not find GSASIIscriptable.py")

    def test_rietveld_refinement(self):
        pass

    def test_pawley_refinement(self):
        pass


if __name__ == '__main__':
    unittest.main()
