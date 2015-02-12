"""
Test the use of proxy objects in MantidPlot that
prevent crashes when accessing the python object
after deletion of the object
"""
import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtGui, QtCore

# =============== Create a fake workspace to plot =======================
X1 = np.linspace(0,10, 100)
Y1 = 1000*(np.sin(X1)**2) + X1*10
X1 = np.append(X1, 10.1)

X2 = np.linspace(2,12, 100)
Y2 = 500*(np.cos(X2/2.)**2) + 20
X2 = np.append(X2, 12.10)

X = np.append(X1, X2)
Y = np.append(Y1, Y2)
E = np.sqrt(Y)

CreateWorkspace(OutputWorkspace="fake", DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2, UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")
LoadRaw(Filename=r'IRS26173.raw',OutputWorkspace='IRS26173',Cache='Always',LoadLogFiles='0',LoadMonitors='Exclude')

class MantidPlotProxiesTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        """Clean up by closing the created window """
        pass

    def try_closing(self, obj, msg=""):
        """ Try closing a graphical object, and
        access the variable to see if it has been set to None """
        self.assertFalse(obj._getHeldObject() is None, msg + "'s return value was not None to begin with")
        # No closing dialog
        obj.confirmClose(False)
        # This should close (and hopefully delete) obj
        obj.close()
        # Make sure the event passes
        QtCore.QCoreApplication.processEvents()
        QtCore.QCoreApplication.processEvents()
        # Check that the object has been None'd
        self.assertTrue(obj._getHeldObject() is None, msg + "'s return value gets cleared when closed.")

    def test_closing_retrieved_object(self):
        """Create object using newXXX("name"), retrieve it using XXX("name") and then close it """
        for cmd in ['table']:#, 'matrix', 'graph', 'note']:
            name = "testobject%s" % cmd
            # Create a method called newTable, for example
            newMethod = "new" + cmd[0].upper() + cmd[1:] + '("%s")' % name
            eval(newMethod)
            obj = eval(cmd + '("%s")' % name)
            self.try_closing(obj, cmd+"()")

    def test_closing_newTable(self):
        obj = newTable()
        self.try_closing(obj, "newTable()")

    def test_closing_newMatrix(self):
        obj = newMatrix()
        self.try_closing(obj, "newMatrix()")

    def test_closing_newPlot3D(self):
        obj = newPlot3D()
        self.try_closing(obj, "newPlot3D()")

    def test_closing_newNote(self):
        obj = newNote()
        self.try_closing(obj, "newNote()")

    def test_closing_newGraph(self):
        obj = newGraph()
        self.try_closing(obj, "newGraph()")

    def test_closing_layers(self):
        g = newGraph()
        l1= g.layer(1)
        l2 = g.addLayer()
        l_active = g.activeLayer()
        self.try_closing(g, "newGraph()")
        self.assertTrue(l1._getHeldObject() is None, "Layer object 0 from deleted graph is None")
        self.assertTrue(l2._getHeldObject() is None, "Layer object 1 from deleted graph is None")
        self.assertTrue(l_active._getHeldObject() is None, "Active Layer object from deleted graph is None")

    def test_closing_Layer_objects(self):
        """ Make a plot then access some contained objects.
        They should safely be cleared when deleting the graph"""
        g = plotSpectrum("fake", [0,1])
        g.confirmClose(False)
        l = g.activeLayer()
        legend = l.legend()
        legend2 = l.newLegend("a new legend")
        grid = l.grid()
        errbar = l.errorBarSettings(0)
        self.assertFalse(legend._getHeldObject() is None, "Object returned correctly")
        self.assertFalse(legend2._getHeldObject() is None, "Object returned correctly")
        self.assertFalse(grid._getHeldObject() is None, "Object returned correctly")
        self.assertFalse(l._getHeldObject() is None, "Object returned correctly")
        self.assertFalse(errbar._getHeldObject() is None, "Object returned correctly")
        # Deleting the parent graph should None the children
        self.try_closing(g, "plotSpectrum()")
        self.assertTrue(legend._getHeldObject() is None, "Deleted Legend safely")
        self.assertTrue(legend2._getHeldObject() is None, "Deleted new Legend safely")
        self.assertTrue(grid._getHeldObject() is None, "Deleted Grid safely")
        self.assertTrue(l._getHeldObject() is None, "Deleted Layer safely")
        self.assertTrue(errbar._getHeldObject() is None, "Deleted ErrorBarSettings safely")

    def test_closing_MantidMatrix(self):
        """ Create a MantidMatrix and then delete it safely """
        mm = importMatrixWorkspace("fake", visible=True)
        self.try_closing(mm, "importMatrixWorkspace()")

    def test_closing_MantidMatrix_plotGraph2D(self):
        """ Make a color fill plot. then delete"""
        mm = importMatrixWorkspace("fake", visible=True)
        g = mm.plotGraph2D()
        spec = g.activeLayer().spectrogram()
        self.try_closing(mm, "importMatrixWorkspace()")
        self.assertTrue(g._getHeldObject() is None, "Deleted graph safely when the parent MantidMatrix was deleted")
        self.assertTrue(spec._getHeldObject() is None, "Deleted spectrogram safely")

    def test_closing_MantidMatrix_plotGraph3D(self):
        """ Make a 3D plot. then delete"""
        mm = importMatrixWorkspace("fake", visible=True)
        g = mm.plotGraph3D()
        self.try_closing(mm, "importMatrixWorkspace()")
        self.try_closing(g, "importMatrixWorkspace().plotGraph3D()")

    def test_closing_getInstrumentView(self):
        iv = getInstrumentView("IRS26173")
        self.try_closing(iv, "getInstrumentView()")

    def test_convertToWaterfall(self):
        g = plot(workspace("IRS26173"),(0,1,2,3,4))
        convertToWaterfall(g)
        self.try_closing(g, "convertToWaterfall()")

    def test_dock_method_produces_docked_window_on_matrix(self):
        self.do_dock_test_and_close(importMatrixWorkspace("fake", visible=True))

    def test_dock_method_produces_docked_window_on_1D_graph(self):
        self.do_dock_test_and_close(plotSpectrum("fake", 0))

    def test_dock_method_produces_docked_window_on_2D_graph(self):
        mm = importMatrixWorkspace("fake", visible=True)
        g = mm.plotGraph2D()
        self.do_dock_test_and_close(g)
        mm.confirmClose(False)
        mm.close()

    def test_dock_method_produces_docked_window_on_instrument_view(self):
        iv = getInstrumentView("IRS26173")
        self.do_dock_test_and_close(iv)

    def do_dock_test_and_close(self, win):
        win.dock()
        self.assertTrue(win.isDocked())
        win.confirmClose(False)
        win.close()


# Run the unit tests
mantidplottests.runTests(MantidPlotProxiesTest)

