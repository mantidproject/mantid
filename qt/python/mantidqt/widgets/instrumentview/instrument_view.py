#!/usr/bin/env python

import functools
from enum import IntEnum
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter


def safe_qthread(func):
    @functools.wraps(func)
    def _wrapped(*args, **kwargs):
        return QAppThreadCall(func)(*args, **kwargs)

    return _wrapped


class SurfaceType(IntEnum):
    Full3D = 0
    CylindricalX = 1
    CylindricalY = 2
    CylindricalZ = 3
    SphericalX = 4
    SphericalY = 5
    SphericalZ = 6
    SideBySide = 7


class TabName(IntEnum):
    Render = 0
    Pick = 1
    Draw = 2
    Instrument = 3


class pyInstrumentView:
    def __init__(self, workspace):
        self._ws = workspace
        self.presenter = self._get_presenter()
        self.rendertab = self._get_rendertab()

    @safe_qthread
    def show_view(self):
        return self.presenter.show_view()

    @safe_qthread
    def _get_presenter(self):
        return InstrumentViewPresenter(self._ws)

    @safe_qthread
    def _get_rendertab(self):
        return self.presenter.get_render_tab()

    @safe_qthread
    def _turn_off_autoscaling(self):
        return self.rendertab.setColorMapAutoscaling(False)

    @safe_qthread
    def set_axis(self, newaxis):
        return self.rendertab.setAxis(newaxis)

    @safe_qthread
    def set_tof_range(self, minval, maxval):
        return self.presenter.container.set_range(minval, maxval)

    @safe_qthread
    def set_intensity_min(self, val):
        self._turn_off_autoscaling()
        return self.rendertab.setMinValue(val, True)

    @safe_qthread
    def set_intensity_max(self, val):
        self._turn_off_autoscaling()
        return self.rendertab.setMaxValue(val, True)

    def set_intensity_range(self, minval, maxval):
        self.set_intensity_min(minval)
        self.set_intensity_max(maxval)

    @safe_qthread
    def select_tab(self, tab_name):
        assert isinstance(tab_name, TabName)
        return self.presenter.container.select_tab(int(tab_name))

    @safe_qthread
    def select_surface_type(self, surface_type):
        assert isinstance(surface_type, SurfaceType)
        return self.rendertab.setSurfaceType(int(surface_type))


if __name__ == "__main__":
    from mantid.simpleapi import LoadEventNexus, mtd

    nexus_path = (
        "/SNS/EQSANS/shared/sans-backend/data/new/ornl/sans/hfir/gpsans/CG2_9177.nxs.h5"
    )
    ws = LoadEventNexus(Filename=nexus_path, NumberOfBins=10)

    myiv = pyInstrumentView(ws)
    myiv.show_view()
