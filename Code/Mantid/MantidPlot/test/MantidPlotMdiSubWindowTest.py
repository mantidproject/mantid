"""
Test of basic 1D plotting methods in MantidPlot
"""
import mantidplottests
from mantidplottests import *
from mantidplot import *
from PyQt4 import QtGui, QtCore

class MantidPlotMdiSubWindowTest(unittest.TestCase):

    def test_table(self):

        self.doTest( newTable() )

    def test_graph(self):

        self.doTest( newGraph() )

    def doTest(self, w):
        if w.isFloating():
            w.dock()
        self.assertFalse( w.isFloating() )
        self.assertTrue( w.isDocked() )
        size = w.size()
        w.undock()
        self.assertTrue( w.isFloating() )
        self.assertFalse( w.isDocked() )
        w.dock()
        self.assertFalse( w.isFloating() )
        self.assertTrue( w.isDocked() )
        # TODO: sizes are not equal. Should we fix it?
        # self.assertEqual( size, w.size() )
        w.close()


# Run the unit tests
mantidplottests.runTests(MantidPlotMdiSubWindowTest)

