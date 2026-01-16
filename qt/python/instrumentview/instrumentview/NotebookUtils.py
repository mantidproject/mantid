# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.NotebookView import NotebookView
from instrumentview.NotebookPresenter import NotebookPresenter
from mantid.dataobjects import Workspace2D
from mantid.simpleapi import Load
from pathlib import Path
import pyvista as pv


def _load_file(file_path: Path) -> Workspace2D:
    ws = Load(str(file_path))
    if (
        not ws.getInstrument()
        or not ws.getInstrument().getName()
        or not ws.getAxis(1).isSpectra()
        or (ws.detectorInfo().detectorIDs().size == 0)
    ):
        raise RuntimeError(f"Could not open instrument for {file_path}. Check that instrument and detectors are present in the workspace.")
    return ws


def create_notebook_window(file_path_or_workspace: Path | str | Workspace2D) -> NotebookView:
    if isinstance(file_path_or_workspace, Path) or isinstance(file_path_or_workspace, str):
        ws = _load_file(file_path_or_workspace)
    elif isinstance(file_path_or_workspace, Workspace2D):
        ws = file_path_or_workspace
    else:
        raise TypeError("Unknown type, must be a Path, str, or Workspace2D")
    pv.set_jupyter_backend("trame")
    model = FullInstrumentViewModel(ws)
    window = NotebookView()
    NotebookPresenter(window, model)
    return window
