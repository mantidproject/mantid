# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
System test: SNAP pixel_assembly IDF produces results identical to the
rectangular_detector IDF when processing real SNAP event data.

Workflow (mirroring the non-DetCal path in SNAPReduce):
  1. Load SNAP run file.
  2. Override embedded instrument with a known IDF via LoadInstrument.
  3. Create bank grouping from the same IDF.
  4. Focus with AlignAndFocusPowder.

The pixel_assembly (PA) and rectangular_detector (RD) outputs are compared;
they must be identical, proving PA is a transparent memory-saving replacement.
"""

import systemtesting
from mantid.simpleapi import (
    AlignAndFocusPowder,
    CreateGroupingWorkspace,
    Load,
    LoadInstrument,
)

_RUN_FILE = "SNAP_45874.nxs.h5"
_RD_IDF = "SNAP_Definition.xml"
_PA_IDF = "SNAP_Definition_PA.xml"
_BINNING = [0.5, -0.004, 7.0]
_COMPRESS_TOL = 0.01


def _load_and_override(run_file, idf_file, ws_out):
    """Load a SNAP NeXus file and replace its embedded instrument."""
    Load(Filename=run_file, OutputWorkspace=ws_out)
    LoadInstrument(Workspace=ws_out, Filename=idf_file, RewriteSpectraMap=False)


def _make_grouping(idf_file, grp_out):
    """Create one group per bank from the given IDF."""
    CreateGroupingWorkspace(
        InstrumentFilename=idf_file,
        GroupDetectorsBy="bank",
        OutputWorkspace=grp_out,
    )


def _focus(raw_ws, grp_ws, focused_out):
    AlignAndFocusPowder(
        InputWorkspace=raw_ws,
        OutputWorkspace=focused_out,
        GroupingWorkspace=grp_ws,
        Params=_BINNING,
        CompressTolerance=_COMPRESS_TOL,
    )


class SNAPPixelAssemblyMatchesRD(systemtesting.MantidSystemTest):
    """
    SNAP_Definition_PA.xml (pixel_assembly) must produce a focused powder
    pattern identical to SNAP_Definition.xml (rectangular_detector) for the
    same SNAP event data.
    """

    def requiredFiles(self):
        return [_RUN_FILE]

    def runTest(self):
        # Reference path: rectangular_detector IDF
        _load_and_override(_RUN_FILE, _RD_IDF, "snap_rd_raw")
        _make_grouping(_RD_IDF, "snap_rd_grp")
        _focus("snap_rd_raw", "snap_rd_grp", "snap_rd_focused")

        # PA path: pixel_assembly IDF
        _load_and_override(_RUN_FILE, _PA_IDF, "snap_pa_raw")
        _make_grouping(_PA_IDF, "snap_pa_grp")
        _focus("snap_pa_raw", "snap_pa_grp", "snap_pa_focused")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        # PA output must exactly match RD output
        return "snap_pa_focused", "snap_rd_focused"
