# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from contextlib import contextmanager

from qtpy.QtGui import QSurfaceFormat


@contextmanager
def preserve_default_surface_format():
    """Undo pyvistaqt's overwrite of the process-wide default QSurfaceFormat.

    ``QVTKRenderWindowInteractor.__init__`` (pyvistaqt >= 0.13) calls
    ``QSurfaceFormat.setDefaultFormat`` with an OpenGL 3.2 core profile. Workbench sets a
    compatibility profile at startup because the legacy Instrument View uses fixed-function
    OpenGL, so leaving the core profile in place makes every QOpenGLWidget created afterwards
    fail to render. A QOpenGLWidget captures the default format when it is constructed, so
    restoring it here leaves the plotter's own widget untouched.
    """
    saved = QSurfaceFormat.defaultFormat()
    try:
        yield
    finally:
        QSurfaceFormat.setDefaultFormat(saved)
