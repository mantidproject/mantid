#!/usr/bin/env python

import functools
from enum import IntEnum
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter


def safe_qthread(func):
    """decorator function to move given call to main QappThread"""
    @functools.wraps(func)
    def _wrapped(*args, **kwargs):
        return QAppThreadCall(func)(*args, **kwargs)

    return _wrapped


class SurfaceType(IntEnum):
    """Enumertor for the surface type in Instrument View"""
    Full3D = 0
    CylindricalX = 1
    CylindricalY = 2
    CylindricalZ = 3
    SphericalX = 4
    SphericalY = 5
    SphericalZ = 6
    SideBySide = 7


class TabName(IntEnum):
    """Enumerator for the Tab in Instrument View"""
    Render = 0
    Pick = 1
    Draw = 2
    Instrument = 3


class pyInstrumentView:
    """
    Python control for the Qt-Based instrument viewer app

    Note: once the app window is closed, the Qt app is purged
          from the memory.  To resume control, a new instance of this class
          needs to be instantiated to generate a new viewer app.
    """
    def __init__(self, workspace):
        """generate a viewer app with given workspace as its data
        :param workspace: input workspace for viewing
        """
        self._ws = workspace
        self.presenter = self._get_presenter()
        self.rendertab = self._get_rendertab()

    @safe_qthread
    def show_view(self):
        """Display the app"""
        return self.presenter.show_view()

    @safe_qthread
    def _get_presenter(self):
        """get the qt handle to current instrument viewer"""
        return InstrumentViewPresenter(self._ws)

    @safe_qthread
    def _get_rendertab(self):
        """get the handle to the render tab"""
        return self.presenter.get_render_tab()

    @safe_qthread
    def _turn_off_autoscaling(self):
        """turn off auto scaling to make colormap range editable"""
        return self.rendertab.setColorMapAutoscaling(False)

    @safe_qthread
    def set_axis(self, newaxis):
        """set the displaying axis
        :param newaxis: ["Z+"|"Z-"|"X+"|"X-"|"Y+"|"Y-"]
        """
        return self.rendertab.setAxis(newaxis)

    @safe_qthread
    def set_x_range(self, minval, maxval):
        """set the integration range
        :param minval: low bound of the integration range
        :param maxval: max bound of the integration range
        """
        return self.presenter.container.set_range(minval, maxval)

    @safe_qthread
    def set_intensity_min(self, val):
        """set intensity minimum
        :param val: value of the minimum intensity
        """
        self._turn_off_autoscaling()
        return self.rendertab.setMinValue(val, True)

    @safe_qthread
    def set_intensity_max(self, val):
        """set intensity maximum
        :param val: value of the maximum intensty
        """
        self._turn_off_autoscaling()
        return self.rendertab.setMaxValue(val, True)

    def set_intensity_range(self, minval, maxval):
        """set the intensity
        :param minval: minimum intensity
        :param maxval: maximum intesnity
        """
        self.set_intensity_min(minval)
        self.set_intensity_max(maxval)

    @safe_qthread
    def select_tab(self, tab_name):
        """select active tab in the instrument view app
        :param tab_name: tab name, [TabName.Render|TabName.Pick|TabName.Draw|TabName.Instrument]
        """
        assert isinstance(tab_name, TabName)
        return self.presenter.container.select_tab(int(tab_name))

    @safe_qthread
    def select_surface_type(self, surface_type):
        """select surafce type (projection style)
        :param surface_type: [SurfaceType.Full3D | 
                              SurfaceType.CylindricalX | SurfaceType.CylindricalY | SurfaceType.CylindricalZ |
                              SurfaceType.SphericalX   | SurfaceType.SphericalY   | SurfaceType.SphericalZ   |
                              SurfaceType.SideBySide]
        """
        assert isinstance(surface_type, SurfaceType)
        return self.rendertab.setSurfaceType(int(surface_type))


if __name__ == "__main__":
    from mantid.simpleapi import LoadEventNexus

    nexus_path = (
        "/SNS/EQSANS/shared/sans-backend/data/new/ornl/sans/hfir/gpsans/CG2_9177.nxs.h5"
    )
    ws = LoadEventNexus(Filename=nexus_path, NumberOfBins=10)

    myiv = pyInstrumentView(ws)
    myiv.show_view()
