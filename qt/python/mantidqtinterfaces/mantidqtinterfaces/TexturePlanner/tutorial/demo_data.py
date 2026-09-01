# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The sample the tutorial loads, and somewhere to export it to.

Generated rather than shipped. A tutorial that needed a file from the external data store would
skip itself for anyone who had not built that target - which is very nearly everyone the tutorial
is for. A cuboid is also the clearest thing to watch rotate in the lab view.

Everything lives in a temporary directory that goes when the tour does, which is what makes the
export step safe to actually perform: the tour writes a real file, using the real export path, to
somewhere disposable.
"""

import os
import tempfile

from Engineering.common.xml_shapes import get_cube_xml

# 2 cm on a side: comfortably larger than the 4 mm gauge volume preset the tour selects, so the
# gauge volume is visibly a small region inside the sample rather than swallowing it
CUBE_SIDE_M = 0.02

CUBE_NAME = "tutorial_sample"


class DemoData:
    """Files for the tour to load, in a directory that cleans itself up."""

    def __init__(self):
        # ignore_cleanup_errors because a file finder or a loader can still hold a handle open on
        # Windows when the tour ends, and failing to remove a temporary directory is not something
        # to interrupt the user about
        self._tmpdir = tempfile.TemporaryDirectory(prefix="texture_planner_tutorial_", ignore_cleanup_errors=True)
        self.directory = self._tmpdir.name

        self.cube_xml_path = self._write("tutorial_sample.xml", get_cube_xml(CUBE_NAME, CUBE_SIDE_M))

        self.save_directory = os.path.join(self.directory, "output")
        os.makedirs(self.save_directory, exist_ok=True)
        self.save_filename = "tutorial_orientations"

    def _write(self, name, contents):
        path = os.path.join(self.directory, name)
        with open(path, "w") as handle:
            handle.write(contents)
        return path

    def cleanup(self):
        self._tmpdir.cleanup()
