# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from functools import partial
import io
import systemtesting
import os
import sys
import shutil
import tempfile
from threading import Thread
from unittest.mock import patch

from matplotlib.colors import Normalize
from numpy import hstack

from mantidqt.widgets.sliceviewer.views.dataview import SCALENORM
from mantid.simpleapi import (
    CreateMDHistoWorkspace,
    CreateMDWorkspace,
    CreateSampleWorkspace,
    DeleteWorkspace,
    FakeMDEventData,
    BinMD,
    ConvertToDistribution,
    Scale,
    SetUB,
    RenameWorkspace,
    ClearUB,
    LoadMD,
    SaveMD,
    AddSampleLog,
    SetMDFrame,
    CreatePeaksWorkspace,
    CopySample,
    AddPeakHKL,
)
from mantid.api import AnalysisDataService
from mantidqt.utils.qt.testing import get_application
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText
from math import inf
from numpy.testing import assert_allclose


class MockConfig(object):
    def get(self, name, type=None):
        if name == SCALENORM:
            return "Log"

    def set(self, name):
        pass

    def has(self, name):
        if name == SCALENORM:
            return True


def create_histo_ws():
    histo_ws = CreateMDHistoWorkspace(
        Dimensionality=3,
        Extents="-3,3,-10,10,-1,1",
        SignalInput=range(100),
        ErrorInput=range(100),
        NumberOfBins="5,5,4",
        Names="Dim1,Dim2,Dim3",
        Units="MomentumTransfer,EnergyTransfer,Angstrom",
        OutputWorkspace="ws_MD_2d",
    )
    return histo_ws


def create_histo_ws_positive():
    histo_ws_positive = CreateMDHistoWorkspace(
        Dimensionality=3,
        Extents="-3,3,-10,10,-1,1",
        SignalInput=range(1, 101),
        ErrorInput=range(100),
        NumberOfBins="5,5,4",
        Names="Dim1,Dim2,Dim3",
        Units="MomentumTransfer,EnergyTransfer,Angstrom",
        OutputWorkspace="ws_MD_2d_pos",
    )
    return histo_ws_positive


def create_hkl_ws():
    hkl_ws = CreateMDWorkspace(
        Dimensions=3,
        Extents="-10,10,-9,9,-8,8",
        Names="A,B,C",
        Units="r.l.u.,r.l.u.,r.l.u.",
        Frames="HKL,HKL,HKL",
        OutputWorkspace="hkl_ws",
    )
    expt_info = CreateSampleWorkspace()
    hkl_ws.addExperimentInfo(expt_info)
    SetUB(hkl_ws, 1, 1, 1, 90, 90, 90)
    return hkl_ws


class HelperTestingClass(QtWidgetFinder):
    """
    Base class for system tests for testing sliceviewer integration with qt and matplotlib.
    """

    def __init__(self):
        self._qapp = get_application()

    def cleanup(self):
        for ii in self._qapp.topLevelWidgets():
            ii.close()
        self._qapp.sendPostedEvents()
        self._qapp.sendPostedEvents()
        self.assert_no_toplevel_widgets()
        self._qapp = None

    # private methods
    def _assertNoErrorInADSHandlerFromSeparateThread(self, operation):
        """Check the error stream for any errors when calling operation.
        Raise an assertion error if errors are detected
        """
        try:
            # the ads handler catches all exceptions so that the handlers don't
            # bring down the sliceviewer. Check if anything is written to stderr
            stderr_capture = io.StringIO()
            stderr_orig = sys.stderr
            sys.stderr = stderr_capture
            op_thread = Thread(target=operation)
            op_thread.start()
            op_thread.join()
            self.assertTrue("Error occurred in handler" not in stderr_capture.getvalue(), msg=stderr_capture.getvalue())
        finally:
            sys.stderr = stderr_orig


class SliceViewerTestDeleteOnClose(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        histo_ws = create_histo_ws()
        pres = SliceViewer(histo_ws)
        self.assert_widget_created()
        pres.view.close()

        self._qapp.sendPostedEvents()
        self._qapp.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)


class SliceViewerTestNonorthogonalViewDisablesLineplots(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        hkl_ws = create_hkl_ws()
        pres = SliceViewer(hkl_ws)
        line_plots_action, region_sel_action, non_ortho_action = toolbar_actions(
            pres, (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION, ToolItemText.NONORTHOGONAL_AXES)
        )
        line_plots_action.trigger()
        self._qapp.sendPostedEvents()

        non_ortho_action.trigger()
        self._qapp.sendPostedEvents()

        self.assertTrue(non_ortho_action.isChecked())
        self.assertFalse(line_plots_action.isChecked())
        self.assertFalse(line_plots_action.isEnabled())
        self.assertFalse(region_sel_action.isChecked())
        self.assertFalse(region_sel_action.isEnabled())

        pres.view.close()


class SliceViewerTestNonorthogonalViewDisabledWhenEaxisViewed(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws_4D = CreateMDWorkspace(
            Dimensions=4,
            Extents=[-1, 1, -1, 1, -1, 1, -1, 1],
            Names="E,H,K,L",
            Frames="General Frame,HKL,HKL,HKL",
            Units="meV,r.l.u.,r.l.u.,r.l.u.",
        )
        expt_info_4D = CreateSampleWorkspace()
        ws_4D.addExperimentInfo(expt_info_4D)
        SetUB(ws_4D, 1, 1, 2, 90, 90, 120)
        pres = SliceViewer(ws_4D)
        self._qapp.sendPostedEvents()

        non_ortho_action = toolbar_actions(pres, [ToolItemText.NONORTHOGONAL_AXES])[0]
        self.assertFalse(non_ortho_action.isEnabled())

        pres.view.close()


class SliceViewerTestRegionSelectorDisabledWithLineplots(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        hkl_ws = create_hkl_ws()
        pres = SliceViewer(hkl_ws)
        line_plots_action, region_sel_action = toolbar_actions(pres, (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION))
        line_plots_action.trigger()
        region_sel_action.trigger()
        self._qapp.sendPostedEvents()

        line_plots_action.trigger()
        self._qapp.sendPostedEvents()

        self.assertFalse(line_plots_action.isChecked())
        self.assertFalse(region_sel_action.isChecked())

        pres.view.close()


class SliceViewerTestClimPreventsNegativeValuesIfLognorm(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        histo_ws = create_histo_ws()
        pres = SliceViewer(histo_ws)
        colorbar = pres.view.data_view.colorbar
        colorbar.autoscale.setChecked(False)
        colorbar.norm.setCurrentText("Log")
        prev_bottom_limit = colorbar.cmin.text()
        prev_top_limit = colorbar.cmax.text()

        colorbar.cmin.textChanged.emit(str(-10))
        colorbar.cmax.textChanged.emit(str(-5))

        self.assertEqual(colorbar.cmin.text(), prev_bottom_limit)
        self.assertEqual(colorbar.cmax.text(), prev_top_limit)

        pres.view.close()


class SliceViewerTestNormSwitchesIfContainsNonPositiveData(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        conf = MockConfig()
        histo_ws = create_histo_ws()
        pres = SliceViewer(histo_ws, conf=conf)
        colorbar = pres.view.data_view.colorbar
        self.assertTrue(isinstance(colorbar.get_norm(), Normalize))
        pres.view.close()


class SliceViewerTestChangingNormUpdatesClimValidators(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        histo_ws_positive = create_histo_ws_positive()
        pres = SliceViewer(histo_ws_positive)
        colorbar = pres.view.data_view.colorbar
        colorbar.autoscale.setChecked(False)

        colorbar.norm.setCurrentText("Log")
        self.assertGreaterThan(colorbar.cmin.validator().bottom(), 0)

        colorbar.norm.setCurrentText("Linear")
        self.assertEqual(colorbar.cmin.validator().bottom(), -inf)

        pres.view.close()


class SliceViewerTestUpdatePlotDataUpdatesAxesLimits(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        hkl_ws = create_hkl_ws()
        pres = SliceViewer(hkl_ws)

        # not transpose
        pres.view.data_view.dimensions.transpose = False
        pres.update_plot_data()
        extent = pres.view.data_view.image.get_extent()
        assert_allclose(extent, [-10.0, 10.0, -9.0, 9.0])

        # transpose
        pres.view.data_view.dimensions.transpose = True
        pres.update_plot_data()
        extent = pres.view.data_view.image.get_extent()

        # Should be the same as before as transposition is handled in the model
        assert_allclose(extent, [-10.0, 10.0, -9.0, 9.0])
        assert_allclose(extent[0:2], list(pres.view.data_view.ax.get_xlim()))
        assert_allclose(extent[2:], list(pres.view.data_view.ax.get_ylim()))

        pres.view.close()


class SliceViewerTestViewClosedOnWorkspaceDeleted(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        DeleteWorkspace(ws)

        self._qapp.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)


class SliceViewerTestViewNotClosedOnOtherWorkspaceDeleted(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        other_ws = CreateSampleWorkspace()
        DeleteWorkspace(other_ws)

        self._qapp.sendPostedEvents()

        # Strange behaviour between tests where views don't close properly means
        # we cannot use self.assert_widget_exists, as many instances of SliceViewerView
        # may be active at this time. As long as this view hasn't closed we are OK.
        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        pres.view.close()


class SliceViewerTestViewClosedOnReplaceWhenModelPropertiesChange(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        ConvertToDistribution(ws)

        self._qapp.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)


class SliceViewerTestViewUpdatedOnReplaceWhenModelPropertiesNotChangedMatrixWs(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        def scale_ws(ws):
            ws = Scale(ws, 100, "Multiply")

        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        self._assertNoErrorInADSHandlerFromSeparateThread(partial(scale_ws, ws))

        self._qapp.sendPostedEvents()

        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        pres.view.close()


class SliceViewerTestViewUpdatedOnReplaceWhenModelPropertiesNotChangedMDEventWs(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        def scale_ws(ws):
            ws = ws * 100

        ws = CreateMDWorkspace(
            Dimensions="3", EventType="MDEvent", Extents="-10,10,-5,5,-1,1", Names="Q_lab_x,Q_lab_y,Q_lab_z", Units="1\\A,1\\A,1\\A"
        )
        FakeMDEventData(ws, UniformParams="1000000")
        pres = SliceViewer(ws)
        self._assertNoErrorInADSHandlerFromSeparateThread(partial(scale_ws, ws))

        self._qapp.sendPostedEvents()

        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        pres.view.close()


class SliceViewerTestViewTitleOnWorkspaceRename(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        old_title = pres.model.get_title()

        renamed = RenameWorkspace(ws)  # noqa F841

        self._qapp.sendPostedEvents()

        # View didn't close
        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        # Window title correct
        self.assertNotEqual(pres.model.get_title(), old_title)
        self.assertEqual(pres.view.windowTitle(), pres.model.get_title())

        pres.view.close()


class SliceViewerTestViewTitleNotChangedOnOtherWorkspaceRename(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        title = pres.model.get_title()
        other_workspace = CreateSampleWorkspace()
        other_renamed = RenameWorkspace(other_workspace)  # noqa F841

        self.assertEqual(pres.model.get_title(), title)
        self.assertEqual(pres.view.windowTitle(), pres.model.get_title())


class SliceViewerTestViewClosedOnADSCleared(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        SliceViewer(ws)
        AnalysisDataService.clear()

        self._qapp.sendPostedEvents()

        self.assert_no_toplevel_widgets()


class SliceViewerTestClosedOnReplaceWhenChangedNonorthogonalTransformSupport(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws_non_ortho = CreateMDWorkspace(
            Dimensions="3",
            Extents="-6,6,-4,4,-0.5,0.5",
            Names="H,K,L",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
            SplitInto="2",
            SplitThreshold="50",
        )
        expt_info_nonortho = CreateSampleWorkspace()
        ws_non_ortho.addExperimentInfo(expt_info_nonortho)
        SetUB(ws_non_ortho, 1, 1, 2, 90, 90, 120)
        pres = SliceViewer(ws_non_ortho)
        ClearUB(ws_non_ortho)

        self._qapp.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)


class SliceViewerTestPlotMatrixXlimitsIgnoresMonitors(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        xmin = 5000
        xmax = 10000
        ws = CreateSampleWorkspace(NumBanks=1, NumMonitors=1, BankPixelWidth=1, XMin=xmin, XMax=xmax)
        ws.setX(0, 2 * ws.readX(0))  # change x limits of monitor spectrum
        pres = SliceViewer(ws)

        pres.view.data_view.plot_matrix(ws)

        self.assertEqual(pres.view.data_view.get_data_limits_to_fill_current_axes()[0], (xmin, xmax))


class SliceViewerTestPlotMatrixXlimitsIgnoresNans(systemtesting.MantidSystemTest, HelperTestingClass):
    # need to mock update slider as doesn't handle inf when initialising SliceViewer in this manner
    @patch("mantidqt.widgets.sliceviewer.views.dimensionwidget.Dimension.update_slider")
    def runTest(self, mock_update_slider):
        HelperTestingClass.__init__(self)
        xmin = 5000
        xmax = 10000
        ws = CreateSampleWorkspace(NumBanks=2, BankPixelWidth=2, XMin=xmin, XMax=xmax)  # two non-monitor spectra
        ws.setX(0, hstack([2 * ws.readX(0)[0:-1], inf]))  # change x limits of spectrum and put inf in last element
        pres = SliceViewer(ws)

        pres.view.data_view.plot_matrix(ws)

        self.assertEqual(pres.view.data_view.get_data_limits_to_fill_current_axes()[0], (xmin, xmax))


class SliceViewerTestCloseEvent(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        self.assert_widget_created()

        pres.view.close()
        pres = None
        self._qapp.sendPostedEvents()

        self.assert_no_toplevel_widgets()


class SliceViewerTestAxesLimitsRespectNonorthogonalTransform(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        limits = (-10.0, 10.0, -9.0, 9.0)
        ws_nonrotho = CreateMDWorkspace(
            Dimensions=3,
            Extents=",".join([str(lim) for lim in limits]) + ",-8,8",
            Names="A,B,C",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
        )
        expt_info_nonortho = CreateSampleWorkspace()
        ws_nonrotho.addExperimentInfo(expt_info_nonortho)
        SetUB(ws_nonrotho, 1, 1, 2, 90, 90, 120)
        pres = SliceViewer(ws_nonrotho)

        # assert limits of orthog
        limits_orthog = pres.view.data_view.get_data_limits_to_fill_current_axes()
        self.assertEqual(limits_orthog[0], limits[0:2])
        self.assertEqual(limits_orthog[1], limits[2:])

        # set nonorthog view and retrieve new limits
        pres.nonorthogonal_axes(True)
        limits_nonorthog = pres.view.data_view.get_data_limits_to_fill_current_axes()
        self.assertAlmostEqual(limits_nonorthog[0][0], -19, delta=1e-5)
        self.assertAlmostEqual(limits_nonorthog[0][1], 19, delta=1e-5)
        self.assertEqual(limits_nonorthog[1], limits[2:])

        pres.view.close()


class SliceViewerTestLoadMD(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        test_dir = tempfile.mkdtemp()

        ws = CreateMDHistoWorkspace(
            Dimensionality=4,
            Extents="-3,3,-10,10,-5,5,0,2",
            SignalInput=[1] * 11**4,
            ErrorInput=[1] * 11**4,
            NumberOfBins="11,11,11,11",
            Names="values,[H,0,0],[0,K,0],[0,0,L]",
            Units="a.b.u.,r.l.u.,r.l.u.,r.l.u.",
            Frames="General Frame,HKL,HKL,HKL",
        )

        SetMDFrame("ws", MDFrame="HKL", Axes=[1, 2, 3])
        AddSampleLog("ws", LogName="sample")
        ws.getExperimentInfo(0).run().addProperty("W_MATRIX", [1, 0, 0, 0, 1, 0, 0, 0, 1], True)
        SaveMD("ws", Filename=os.path.join(test_dir, "mdhist_4d.nxs"))
        DeleteWorkspace("ws")

        ws = LoadMD(os.path.join(test_dir, "mdhist_4d.nxs"))
        pres = SliceViewer(ws)
        self.assertAlmostEqual(pres.get_proj_matrix().flatten().tolist(), [1, 0, 0, 0, 1, 0, 0, 0, 1])
        pres.view.close()

        shutil.rmtree(test_dir)


class SliceViewerTestIntegerXAxis(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        ws = CreateMDHistoWorkspace(
            Dimensionality=4,
            Extents="-2,2,-2,2,-2,2,-2,2",
            SignalInput=[1] * 11**4,
            ErrorInput=[1] * 11**4,
            NumberOfBins="11,11,11,11",
            Names="[H,0,0],values,[0,K,0],[0,0,L]",
            Units="r.l.u.,a.b.u.,r.l.u.,r.l.u.",
            Frames="HKL,General Frame,HKL,HKL",
        )

        SetMDFrame("ws", MDFrame="HKL", Axes=[0, 2, 3])
        AddSampleLog("ws", LogName="sample")
        ws.getExperimentInfo(0).run().addProperty("W_MATRIX", [1, 0, 0, 0, 1, 0, 0, 0, 1], True)

        pres = SliceViewer(ws)
        xticklocs = pres.view.data_view.ax.get_xticks()
        yticklocs = pres.view.data_view.ax.get_yticks()
        self.assertTrue((xticklocs % 1 == 0).all())
        self.assertFalse((yticklocs % 1 == 0).all())
        pres.view.close()


class SliceViewerTestIntegerYAxis(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        ws = CreateMDHistoWorkspace(
            Dimensionality=4,
            Extents="-2,2,-2,2,-2,2,-2,2",
            SignalInput=[1] * 11**4,
            ErrorInput=[1] * 11**4,
            NumberOfBins="11,11,11,11",
            Names="values,[H,0,0],[0,K,0],[0,0,L]",
            Units="a.b.u.,r.l.u.,r.l.u.,r.l.u.",
            Frames="General Frame,HKL,HKL,HKL",
        )

        SetMDFrame("ws", MDFrame="HKL", Axes=[1, 2, 3])
        AddSampleLog("ws", LogName="sample")
        ws.getExperimentInfo(0).run().addProperty("W_MATRIX", [1, 0, 0, 0, 1, 0, 0, 0, 1], True)

        pres = SliceViewer(ws)
        xticklocs = pres.view.data_view.ax.get_xticks()
        yticklocs = pres.view.data_view.ax.get_yticks()
        self.assertFalse((xticklocs % 1 == 0).all())
        self.assertTrue((yticklocs % 1 == 0).all())
        pres.view.close()


class SliceViewerTestCreateMDEvent(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        ws = CreateMDWorkspace(
            Dimensions="4",
            EventType="MDEvent",
            Extents="-5.7644,5.7644,-5.7644,5.7644,-5.7644,5.7644,-11.3897,11.3897",
            Names="Q_sample_x,Q_sample_y,Q_sample_z,DeltaE",
            Units="Angstrom^-1,Angstrom^-1,Angstrom^-1,DeltaE",
        )

        SetMDFrame(ws, MDFrame="QSample", Axes=[0, 1, 2])
        FakeMDEventData(ws, UniformParams="1000000")

        binned_ws = BinMD(
            InputWorkspace=ws,
            AxisAligned=False,
            BasisVector0="[H,H,0],unit,-1.4747400283813477,-0.03335599973797798,0.26337599754333496,0",
            BasisVector1="DeltaE,DeltaE,0,0,0,1",
            BasisVector2="[-H,H,0],unit,-0.2630769908428192,-0.01594259962439537,-1.4750800132751465,0",
            BasisVector3="[0,0,L],unit,-0.025200000032782555,1.0592399835586548,-0.006953829899430275,0",
            NormalizeBasisVectors=False,
            OutputExtents=[-3.8441, 3.8559, -2, 8, -0.1, 0.1, -5.43682, 5.43682],
            OutputBins=[154, 100, 1, 218],
        )

        binned_ws.clearOriginalWorkspaces()

        pres = SliceViewer(binned_ws)
        proj_matrix = pres.get_proj_matrix().tolist()
        self.assertAlmostEqual(proj_matrix[0][0], -1.4747400283813477)
        self.assertAlmostEqual(proj_matrix[0][1], -0.2630769908428192)
        self.assertAlmostEqual(proj_matrix[0][2], -0.025200000032782555)
        self.assertAlmostEqual(proj_matrix[1][0], -0.03335599973797798)
        self.assertAlmostEqual(proj_matrix[1][1], -0.01594259962439537)
        self.assertAlmostEqual(proj_matrix[1][2], 1.0592399835586548)
        self.assertAlmostEqual(proj_matrix[2][0], 0.26337599754333496)
        self.assertAlmostEqual(proj_matrix[2][1], -1.4750800132751465)
        self.assertAlmostEqual(proj_matrix[2][2], -0.006953829899430275)
        pres.view.close()


class SliceViewerTestCreateMDHisto(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)

        ws_4D_histo = CreateMDWorkspace(
            Dimensions="4",
            Extents="-3,3,-3,3,-3,3,-1,1",
            Names="H,K,L,E",
            Units="r.l.u.,r.l.u.,r.l.u.,meV",
            Frames="HKL,HKL,HKL,General Frame",
            SplitInto="2",
            SplitThreshold="10",
        )
        expt_info = CreateSampleWorkspace()
        ws_4D_histo.addExperimentInfo(expt_info)
        SetUB(ws_4D_histo, 1, 1, 2, 90, 90, 120)
        binned_ws = BinMD(
            InputWorkspace=ws_4D_histo,
            AxisAligned=False,
            BasisVector0="L,r.l.u.,0,0,1,0",
            BasisVector1="E,meV,0,0,0,1",
            BasisVector2="H,r.l.u.,1,0,0,0",
            BasisVector3="K,r.l.u.,0,1,0,0",
            OutputExtents="-3,3,-1,1,-3,3,-3,3",
            OutputBins="11,11,11,11",
            OutputWorkspace="ws_4D_slice_axis_aligned",
            NormalizeBasisVectors=False,
        )

        binned_ws.clearOriginalWorkspaces()

        pres = SliceViewer(binned_ws)
        self.assertListEqual(pres.get_proj_matrix().tolist(), [[0.0, 1.0, 0.0], [0.0, 0.0, 1.0], [1.0, 0.0, 0.0]])
        pres.view.close()


class SliceViewerTestPeakOutsideOfDataRange(systemtesting.MantidSystemTest, HelperTestingClass):
    def runTest(self):
        HelperTestingClass.__init__(self)
        ws_4D = CreateMDWorkspace(
            Dimensions=4,
            Extents=[-1, 1, -1, 1, -1, 1, -1, 1],
            Names="H,K,L,E",
            Frames="HKL,HKL,HKL,General Frame",
            Units="r.l.u.,r.l.u.,r.l.u.,meV",
        )
        FakeMDEventData(ws_4D, UniformParams=1e6)
        expt_info_4D = CreateSampleWorkspace()
        ws_4D.addExperimentInfo(expt_info_4D)
        SetUB(ws_4D, 1, 1, 2, 90, 90, 120)
        CreatePeaksWorkspace(InstrumentWorkspace="ws_4D", NumberOfPeaks=0, OutputWorkspace="peaks")
        CopySample(
            InputWorkspace="ws_4D", OutputWorkspace="peaks", CopyName=False, CopyMaterial=False, CopyEnvironment=False, CopyShape=False
        )
        AddPeakHKL(Workspace="peaks", HKL="1,2,3")
        sv = SliceViewer(ws_4D)
        sv._create_peaks_presenter_if_necessary().overlay_peaksworkspaces(["peaks"])
        sv.set_slicepoint([None, None, 3.0, 0.0])
        l_spinbox = sv.get_dimensions().dims[2].spinbox
        self.assertEqual(l_spinbox.value(), 3.0)
        self.assertEqual(l_spinbox.minimum(), -0.99)
        self.assertEqual(l_spinbox.maximum(), 3.0)
        sv.view.close()


# private helper functions
def toolbar_actions(presenter, text_labels):
    """
    Return a list of actions with the given text labels
    :param presenter: Presenter containing view
    :param text_labels: A list of text labels on toolbar buttons
    :return: A list of QAction objects matching the text labels given
    """
    toolbar_actions = presenter.view.data_view.mpl_toolbar.actions()
    return [action for action in toolbar_actions if action.text() in text_labels]
