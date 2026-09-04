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

# A cuboid rather than a cube, and deliberately not close to one: a cube looks identical from every
# face, so rotating it in the lab view shows the user almost nothing. With three different edge
# lengths every orientation looks distinct, which is the whole point of watching it turn.
#
# Comfortably larger than the 4 mm gauge volume preset the tour selects, so the gauge volume reads
# as a small region inside the sample rather than swallowing it.
CUBOID_WIDTH_M = 0.030  # x
CUBOID_HEIGHT_M = 0.010  # y
CUBOID_DEPTH_M = 0.020  # z

SAMPLE_NAME = "tutorial_sample"


def get_cuboid_xml(name, width, height, depth, centre=(0.0, 0.0, 0.0)):
    """A CSG cuboid in Mantid's sample-shape XML.

    ``Engineering.common.xml_shapes`` only offers a cube; this is the same shape with the three
    edges given independently.
    """
    return (
        f"<cuboid id='{name}'> "
        f"<height val='{height}' /> "
        f"<width val='{width}' /> "
        f"<depth val='{depth}' /> "
        f"<centre x='{centre[0]}' y='{centre[1]}' z='{centre[2]}' /> "
        f"</cuboid> "
        f"<algebra val='{name}' />"
    )


class DemoData:
    """Files for the tour to load, in a directory that cleans itself up."""

    def __init__(self):
        # ignore_cleanup_errors because a file finder or a loader can still hold a handle open on
        # Windows when the tour ends, and failing to remove a temporary directory is not something
        # to interrupt the user about
        self._tmpdir = tempfile.TemporaryDirectory(prefix="texture_planner_tutorial_", ignore_cleanup_errors=True)
        self.directory = self._tmpdir.name

        self.cuboid_xml_path = self._write(
            "tutorial_sample.xml",
            get_cuboid_xml(SAMPLE_NAME, CUBOID_WIDTH_M, CUBOID_HEIGHT_M, CUBOID_DEPTH_M),
        )

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
