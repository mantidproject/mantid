# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import functools
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantidqt.utils.qt.qappthreadcall import force_method_calls_to_qapp_thread


def safe_qthread(func):
    """decorator function to move given call to main QappThread"""

    @functools.wraps(func)
    def _wrapped(*args, **kwargs):
        return QAppThreadCall(func)(*args, **kwargs)

    return _wrapped


def get_instrumentview(workspace, wait=True):
    """Return a handle to the instrument view of given workspace
    :param ws: input workspace
    """

    def _wrappper(ws):
        return force_method_calls_to_qapp_thread(InstrumentViewPresenter(ws))

    # need to do some duck-typing here
    ivp = QAppThreadCall(_wrappper)(workspace)
    # link nested method to top level
    # NOTE: setMin and setMax still leads to segfault, need to force
    #       wrapped in QAppThreadCall again
    ivp.reset_view = ivp.get_render_tab().resetView
    ivp.select_tab = ivp.container.select_tab
    ivp.select_surface_type = ivp.get_render_tab().setSurfaceType
    ivp.set_maintain_aspect_ratio = ivp.get_render_tab().setMaintainAspectRatio
    ivp.set_auto_scaling = ivp.get_render_tab().setColorMapAutoscaling
    ivp.set_axis = ivp.get_render_tab().setAxis
    ivp.set_bin_range = safe_qthread(ivp.container.widget.setBinRange)
    ivp.set_color_min = safe_qthread(ivp.get_render_tab().setMinValue)
    ivp.set_color_max = safe_qthread(ivp.get_render_tab().setMaxValue)
    ivp.set_color_range = safe_qthread(ivp.get_render_tab().setRange)
    ivp.set_color_scale = ivp.get_render_tab().setLegendScaleType
    ivp.is_thread_running = ivp.container.widget.isThreadRunning
    ivp.wait = ivp.container.widget.waitForThread
    ivp.replace_workspace = safe_qthread(ivp.replace_workspace)
    ivp.save_image = safe_qthread(ivp.save_image)
    # wait for the instrument view finish construction before returning it
    if wait:
        ivp.wait()
    return ivp
