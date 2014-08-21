import sys
import sys
import os
import unittest
import time
from PyQt4 import QtCore, QtGui


# Import the Mantid framework
from mantid.simpleapi import *
import mantidqtpython

from mantidqtpython import StdRuntimeError, StdInvalidArgument

# Create the application only once per test; otherwise I get a segfault
app = QtGui.QApplication(sys.argv)


class SliceViewerPythonInterfaceTest(unittest.TestCase):
    """Test for accessing SliceViewer widgets from MantidPlot
    python interpreter"""

    def setUp(self):
        """ Set up and create a SliceViewer widget """
        # Create a test data set
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',
            Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='mdw')
        FakeMDEventData("mdw",  UniformParams="1e4")
        FakeMDEventData("mdw",  PeakParams="1e3, 1, 2, 3, 1.0")
        BinMD(InputWorkspace="mdw", OutputWorkspace="uniform",  AxisAligned=1, AlignedDim0="x,0,10,30",  AlignedDim1="y,0,10,30",  AlignedDim2="z,0,10,30", IterateEvents="1", Parallel="0")
        CreateWorkspace('workspace2d', '1,2,3', '2,3,4')
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='empty')
        # Get the factory to create the SliceViewerWindow in C++
        self.svw = mantidqtpython.MantidQt.Factory.WidgetFactory.Instance().createSliceViewerWindow("uniform", "")
        # Retrieve the SliceViewer widget alone.
        self.sv = self.svw.getSlicer()
        pass

    def setUpXML(self):
        """Special set up for the XML version """
        CreateMDWorkspace(Dimensions='3',Extents='-15,15, -15,15, -15,15',Names='Q_lab_x,Q_lab_y,Q_lab_z',
            Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='TOPAZ_3680')
        CreateMDWorkspace(Dimensions='4',Extents='-15,15, -15,15, -15,15, -10, 100',Names='Q_x,Q_y,Q_z,E',
            Units='A,A,A,meV',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='WS_4D')
        FakeMDEventData("TOPAZ_3680",  UniformParams="1e4")
        FakeMDEventData("WS_4D",  UniformParams="1e4")

        self.xml_3d = """<MDInstruction><MDWorkspaceName>TOPAZ_3680</MDWorkspaceName>
<DimensionSet>
    <Dimension ID="Q_lab_x"><Name>Q_lab_x</Name><Units>Angstroms^-1</Units><UpperBounds>15.0000</UpperBounds><LowerBounds>-15.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_lab_y"><Name>Q_lab_y</Name><Units>Angstroms^-1</Units><UpperBounds>15.0000</UpperBounds><LowerBounds>-15.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_lab_z"><Name>Q_lab_z</Name><Units>Angstroms^-1</Units><UpperBounds>15.0000</UpperBounds><LowerBounds>-15.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <XDimension><RefDimensionId>Q_lab_x</RefDimensionId></XDimension>
    <YDimension><RefDimensionId>Q_lab_y</RefDimensionId></YDimension>
    <ZDimension><RefDimensionId>Q_lab_z</RefDimensionId></ZDimension>
    <TDimension><RefDimensionId/></TDimension>
</DimensionSet>
<Function><Type>PlaneImplicitFuction</Type>
<ParameterList>
    <Parameter><Type>NormalParameter</Type><Value>1 0 0</Value></Parameter>
    <Parameter><Type>OriginParameter</Type><Value>4.84211 0 0</Value></Parameter>
</ParameterList></Function>
</MDInstruction>"""

        self.xml_4d = """<MDInstruction><MDWorkspaceName>WS_4D</MDWorkspaceName>
<DimensionSet>
    <Dimension ID="Q_x"><Name>Q_x</Name><Units>Ang</Units><UpperBounds>5.7415</UpperBounds><LowerBounds>-1.5197</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_y"><Name>Q_y</Name><Units>Ang</Units><UpperBounds>6.7070</UpperBounds><LowerBounds>-6.6071</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_z"><Name>Q_z</Name><Units>Ang</Units><UpperBounds>6.6071</UpperBounds><LowerBounds>-6.6071</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="E"><Name>E</Name><Units>MeV</Units><UpperBounds>150.0000</UpperBounds><LowerBounds>0.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <XDimension><RefDimensionId>Q_x</RefDimensionId></XDimension>
    <YDimension><RefDimensionId>Q_y</RefDimensionId></YDimension>
    <ZDimension><RefDimensionId>E</RefDimensionId></ZDimension>
    <TDimension><RefDimensionId>Q_z</RefDimensionId><Value>4.567</Value></TDimension>
</DimensionSet>
<Function><Type>PlaneImplicitFuction</Type><ParameterList>
    <Parameter><Type>NormalParameter</Type><Value>0 1 0</Value></Parameter>
    <Parameter><Type>OriginParameter</Type><Value>0 1.234 0</Value></Parameter>
</ParameterList></Function></MDInstruction>"""



    def tearDown(self):
        """ Close the created widget """
        # This is crucial! Forces the object to be deleted NOW, not when python exits
        # This prevents a segfault in Ubuntu 10.04, and is good practice.
        self.svw.deleteLater()
        #self.svw.show()
        # Schedule quit at the next event
        QtCore.QTimer.singleShot(0, app, QtCore.SLOT("quit()"))
        # This is required for deleteLater() to do anything (it deletes at the next event loop)
        app.quitOnLastWindowClosed = True
        app.exec_()


    #==========================================================================
    #======================= Basic Tests ======================================
    #==========================================================================

    def test_setWorkspace(self):
        sv = self.sv
        assert (sv is not None)

    def test_getWorkspace(self):
        sv = self.sv
        self.assertEqual(sv.getWorkspaceName(), "uniform")
        assert (sv is not None)

    def test_setWorkspace_MDEventWorkspace(self):
        sv = self.sv
        sv.setWorkspace('mdw')

    def test_setWorkspace_throwsOnBadInputs(self):
        sv = self.sv
        #sv.setWorkspace('workspace2d')
        self.assertRaises(StdRuntimeError, sv.setWorkspace, '')
        self.assertRaises(StdRuntimeError, sv.setWorkspace, 'non_existent_workspace')
        self.assertRaises(StdRuntimeError, sv.setWorkspace, 'workspace2d')

    #==========================================================================
    #======================= XML Tests ========================================
    #==========================================================================
    def test_openFromXML_3D(self):
        sv = self.sv
        self.setUpXML()
        # Read the XML and set the view
        sv.openFromXML(self.xml_3d)
        # Check the settings
        self.assertEqual(sv.getWorkspaceName(), "TOPAZ_3680")
        self.assertEqual(sv.getDimX(), 1)
        self.assertEqual(sv.getDimY(), 2)
        self.assertAlmostEqual( sv.getSlicePoint(0), 4.84211, 3)
        pass

    def test_openFromXML_4D(self):
        sv = self.sv
        self.setUpXML()

        # Read the XML and set the view
        sv.openFromXML(self.xml_4d)
        # Check the settings
        self.assertEqual(sv.getWorkspaceName(), "WS_4D")
        self.assertEqual(sv.getDimX(), 0) # Q_x is X dimension
        self.assertEqual(sv.getDimY(), 3) # Energy is Y
        self.assertAlmostEqual( sv.getSlicePoint(1), 1.234, 3) # Slice point in Q_y
        self.assertAlmostEqual( sv.getSlicePoint(2), 4.567, 3) # Slice point in Q_z

    def test_openFromXML_3D_binned(self):
        sv = self.sv
        self.setUpXML()
        BinMD(InputWorkspace="TOPAZ_3680", OutputWorkspace="TOPAZ_3680_visual_md",
              AxisAligned=1, AlignedDim0="Q_lab_x,0,10,20", AlignedDim1="Q_lab_y,0,10,20", AlignedDim2="Q_lab_z,0,10,20")
        # Read the XML and set the view
        sv.openFromXML(self.xml_3d)
        # Check the settings
        # Automatically grabbed the histo version
        self.assertEqual(sv.getWorkspaceName(), "TOPAZ_3680_visual_md")
        self.assertEqual(sv.getDimX(), 1)
        self.assertEqual(sv.getDimY(), 2)
        self.assertAlmostEqual( sv.getSlicePoint(0), 4.84211, 3)
        pass


    #==========================================================================
    #======================= Setting Dimensions, etc ==========================
    #==========================================================================
    def test_setXYDim(self):
        sv = self.sv
        sv.setXYDim(0,2)
        self.assertEqual( sv.getDimX(), 0, "X dimension was set")
        self.assertEqual( sv.getDimY(), 2, "Y dimension was set")
        #sv.show()
        #app.exec_()

    def test_setXYDim_strings(self):
        sv = self.sv
        sv.setXYDim("x", "z")
        self.assertEqual( sv.getDimX(), 0, "X dimension was set")
        self.assertEqual( sv.getDimY(), 2, "Y dimension was set")

    def test_setXYDim_strings_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdRuntimeError, sv.setXYDim, "monkey", "y")
        self.assertRaises(StdRuntimeError, sv.setXYDim, "x", "monkey")


    def test_setXYDim_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.setXYDim, -1, 0)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 5, 0)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 0, -1)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 0, 3)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 0, 0)

    def test_setSlicePoint(self):
        sv = self.sv
        # Set the slice point and got back the value?
        sv.setSlicePoint(2, 7.6)
        self.assertAlmostEqual( sv.getSlicePoint(2), 7.6, 2)
        # Go to too small a value
        sv.setSlicePoint(2, -12.3)
        self.assertAlmostEqual( sv.getSlicePoint(2), 0.0, 2)
        # Go to too big a value
        sv.setSlicePoint(2, 22.3)
        self.assertAlmostEqual( sv.getSlicePoint(2), 10.0, 2)

    def test_setSlicePoint_strings(self):
        sv = self.sv
        sv.setSlicePoint("z", 7.6)
        self.assertAlmostEqual( sv.getSlicePoint("z"), 7.6, 2)

    def test_setSlicePoint_strings_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdRuntimeError, sv.setSlicePoint, "monkey", 2.34)
        self.assertRaises(StdRuntimeError, sv.getSlicePoint, "monkey")

    def test_setSlicePoint_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.setSlicePoint, -1, 7.6)
        self.assertRaises(StdInvalidArgument, sv.setSlicePoint, 3, 7.6)

    def test_getSlicePoint_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.getSlicePoint, -1)
        self.assertRaises(StdInvalidArgument, sv.getSlicePoint, 3)

    def test_setXYLimits(self):
        sv = self.sv
        sv.setXYLimits(5,10, 7,8)
        sv.setXYLimits(5,2, 7, 8)
        self.assertEqual(sv.getXLimits(), [5, 2])
        self.assertEqual(sv.getYLimits(), [7, 8])
        #sv.show()
        #app.exec_()

    def test_zoomBy(self):
        sv = self.sv
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])
        # Zoom in by a factor of 2
        sv.zoomBy(2.0)
        self.assertEqual(sv.getXLimits(), [2.5, 7.5])
        self.assertEqual(sv.getYLimits(), [2.5, 7.5])
        # Zoom out to the original size
        sv.zoomBy(0.5)
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])

    def test_setXYCenter(self):
        sv = self.sv
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])
        # Move to a new spot
        sv.setXYCenter(2.0, 6.0)
        self.assertEqual(sv.getXLimits(), [-3, 7])
        self.assertEqual(sv.getYLimits(), [1, 11])

    def test_resetZoom(self):
        sv = self.sv
        sv.zoomBy(2.0)
        self.assertEqual(sv.getXLimits(), [2.5, 7.5])
        self.assertEqual(sv.getYLimits(), [2.5, 7.5])
        # Go back automatically to full range
        sv.resetZoom()
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])

    #==========================================================================
    #======================= ColorMap and range ===============================
    #==========================================================================
    def test_loadColorMap(self):
        """ Needs an absolute path - can't readily do unit test """
        sv = self.sv
        #sv.loadColorMap('')

    def test_setColorScale(self):
        sv = self.sv
        sv.setColorScale(10, 30, False)
        self.assertEqual(sv.getColorScaleMin(), 10)
        self.assertEqual(sv.getColorScaleMax(), 30)
        self.assertEqual(sv.getColorScaleLog(), False)
        sv.setColorScale(20, 1000, True)
        self.assertEqual(sv.getColorScaleMin(), 20)
        self.assertEqual(sv.getColorScaleMax(), 1000)
        self.assertEqual(sv.getColorScaleLog(), True)

    def test_setColorScale_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.setColorScale, 10, 5, False)
        self.assertRaises(StdInvalidArgument, sv.setColorScale, 0, 5, True)
        self.assertRaises(StdInvalidArgument, sv.setColorScale, -3, -1, True)

    def test_setColorScaleAutoFull(self):
        sv = self.sv
        sv.setNormalization(1) # Make sure volume normalization is set
        sv.setColorScaleAutoFull()
        self.assertAlmostEqual(sv.getColorScaleMin(), 27.0, 3)
        self.assertAlmostEqual(sv.getColorScaleMax(), 540.0, 3)

    def test_setColorScaleAutoSlice(self):
        sv = self.sv
        sv.setNormalization(1) # Make sure volume normalization is set
        sv.setColorScaleAutoSlice()
        self.assertAlmostEqual(sv.getColorScaleMin(), 27.0, 3)
        self.assertAlmostEqual(sv.getColorScaleMax(), 81.0, 3)

    def test_setNormalization(self):
        sv = self.sv
        sv.setNormalization(0)
        self.assertEqual(sv.getNormalization(), 0)
        sv.setNormalization(1)
        self.assertEqual(sv.getNormalization(), 1)
        sv.setNormalization(2)
        self.assertEqual(sv.getNormalization(), 2)

    #==========================================================================
    #======================= Screenshots etc. =================================
    #==========================================================================
    def test_setFastRender(self):
        sv = self.sv
        self.assertTrue(sv.getFastRender(), "Fast rendering mode is TRUE by default")
        sv.setFastRender(False)
        self.assertFalse(sv.getFastRender(), "Fast rendering mode is set to false")

    #==========================================================================
    #======================= LineViewer =======================================
    #==========================================================================

    def test_make_a_line(self):
        svw = self.svw
        sv = self.sv
        sv.toggleLineMode(True)
        liner = svw.getLiner()
        liner.setStartXY(1, 1)
        liner.setEndXY(5, 4)
        liner.setNumBins(200)
        liner.apply()
        # Check that the values are there
        self.assertEqual(liner.getNumBins(), 200)
        # Length of 5 with 200 bins = 0.025 width
        self.assertAlmostEqual(liner.getBinWidth(), 0.025, 3)

    def test_setThickness(self):
        svw = self.svw
        self.sv.toggleLineMode(True)
        liner = self.svw.getLiner()
        liner.setPlanarWidth(1.5)
        self.assertAlmostEqual(liner.getPlanarWidth(), 1.5, 3)
        liner.setThickness(2, 0.75)
        # Not yet a method to get the width in any dimension


    def test_fixedBinWidth(self):
        svw = self.svw
        sv = self.sv
        sv.toggleLineMode(True)
        liner = svw.getLiner()
        liner.setFixedBinWidthMode(True, 0.025)
        liner.setStartXY(1, 1)
        liner.setEndXY(5, 4)
        liner.setPlanarWidth(1)
        liner.apply()
        # Length of 5, bin width of 0.025 = 200 bins
        self.assertEqual(liner.getNumBins(), 200)
        self.assertAlmostEqual(liner.getBinWidth(), 0.025, 3)

    #Helper method to find the name of the plot's x axis.
    def _getPlotXAxisName(self, lv, ws):
        index = lv.getXAxisDimensionIndex()
        dim = ws.getDimension(index)
        return dim.getName()

    def test_mdhistoAutoAxisAssignmentWhenNoIntegration(self):
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='A,B,C',Units='A,A,A',OutputWorkspace='original')
        FakeMDEventData(InputWorkspace='original',UniformParams='10000',PeakParams='10000,2,2,2,1',RandomizeSignal='1')
        #Note that all axis have 10 bins below.
        SliceMD(InputWorkspace='original',AlignedDim0='A,0,10,10',AlignedDim1='B,0,10,10',AlignedDim2='C,0,10,10',OutputWorkspace='binned_ws')
        binned_ws = mtd['binned_ws']

        sv = self.sv
        sv.setWorkspace('binned_ws')
        sv.setXYDim("A","B")

        #should toggle to 'A' axis as that is now the longest
        lv = self.svw.getLiner()
        lv.setStartXY(0, 0)
        lv.setEndXY(10,5)
        self.assertEquals("A", self._getPlotXAxisName(lv, binned_ws))

        #should toggle to 'B' axis as that is now the longest
        lv.setStartXY(0, 0)
        lv.setEndXY(5,10)
        self.assertEquals("B", self._getPlotXAxisName(lv, binned_ws))


    def test_mdhistoAutoAxisAssignmentWhenAnAxisIsIntegrated(self):
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='A,B,C',Units='A,A,A',OutputWorkspace='original')
        FakeMDEventData(InputWorkspace='original',UniformParams='10000',PeakParams='10000,2,2,2,1',RandomizeSignal='1')
        #Note that the 'A' axis is now integrated (see call below)
        SliceMD(InputWorkspace='original',AlignedDim0='A,0,10,1',AlignedDim1='B,0,10,10',AlignedDim2='C,0,10,10',OutputWorkspace='binned_ws')
        binned_ws = mtd['binned_ws']

        sv = self.sv
        sv.setWorkspace('binned_ws')
        sv.setXYDim("A","B")
        #should toggle to 'B' axis as the 'A' axis is integrated, even though 'A' is the longest.
        lv = self.svw.getLiner()
        lv.setStartXY(0, 0)
        lv.setEndXY(10,5)
        self.assertEquals("B", self._getPlotXAxisName(lv, binned_ws))

        #should toggle to 'B' axis as that is now the longest and also because 'A' is integrated.
        lv.setStartXY(0, 0)
        lv.setEndXY(5,10)
        self.assertEquals("B", self._getPlotXAxisName(lv, binned_ws))

    def test_mdAutoAxisAssignment(self):
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='A,B,C',Units='A,A,A',OutputWorkspace='original')
        FakeMDEventData(InputWorkspace='original',UniformParams='10000',PeakParams='10000,2,2,2,1',RandomizeSignal='1')
        #Effectively all axis will be 'integrated', nbins = 1 because this workspace is not histogrammed. Plotting functionality should now ignore integration checking on axis to autoplot.
        original = mtd['original']

        sv = self.sv
        sv.setWorkspace('original')
        sv.setXYDim("A","B")
        #should toggle to 'A' axis as the 'A' axis is the longest.
        lv = self.svw.getLiner()
        lv.setStartXY(0, 0)
        lv.setEndXY(10,5)
        self.assertEquals("A", self._getPlotXAxisName(lv, original))

        #should toggle to 'B' axis as that is now the longest.
        lv.setStartXY(0, 0)
        lv.setEndXY(5,10)
        self.assertEquals("B", self._getPlotXAxisName(lv, original))

    #==========================================================================
    #======================= Dynamic Rebinning ================================
    #==========================================================================
    def test_DynamicRebinning(self):
        sv = self.sv
        sv.setRebinThickness(2, 1.0)
        sv.setRebinNumBins(50, 200)
        sv.refreshRebin()
        sv.setRebinMode(True, True)
        time.sleep(1)
        self.assertTrue(mtd.doesExist('uniform_rebinned'), 'Dynamically rebinned workspace was created.')
        ws = mtd['uniform_rebinned']
        self.assertEqual(ws.getNumDims(), 3)
        self.assertEqual(ws.getNPoints(), 50*200*1)





