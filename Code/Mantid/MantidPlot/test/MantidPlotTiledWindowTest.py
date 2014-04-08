""" 
Test the TiledWindow functionality
"""
import mantidplottests
from mantidplottests import *
from mantidplot import *
from PyQt4 import QtGui, QtCore

class MantidPlotTiledWindowTest(unittest.TestCase):
    
    def test_addTile(self):
        
        folder = activeFolder()
        t1 = newTable()
        t2 = newTable()
        self.assertFalse( folder.findWindow(t1.name()) is None )
        self.assertFalse( folder.findWindow(t2.name()) is None )
        tw = newTiledWindow()
        tw.addWidget(t1,0,0)
        tw.addWidget(t2,0,1)
        self.assertTrue( folder.findWindow(t1.name()) is None )
        self.assertTrue( folder.findWindow(t2.name()) is None )
        self.assertEqual( tw.rowCount(), 1 )
        self.assertEqual( tw.columnCount(), 2 )
        tw.close()
        
    def test_removeTile(self):
        
        folder = activeFolder()
        t1 = newTable()
        t2 = newTable()
        tw = newTiledWindow()
        tw.addWidget(t1,0,0)
        tw.addWidget(t2,0,1)
        self.assertTrue( folder.findWindow(t1.name()) is None )
        self.assertTrue( folder.findWindow(t2.name()) is None )
        tw.removeWidgetToDocked(0,0)
        self.assertFalse( folder.findWindow(t1.name()) is None )
        tw.removeWidgetToFloating(0,1)
        self.assertFalse( folder.findWindow(t2.name()) is None )
        self.assertTrue( t1.isDocked() )
        self.assertTrue( t2.isFloating() )
        tw.close()
        t1.close()
        t2.close()
        
# Run the unit tests
mantidplottests.runTests(MantidPlotTiledWindowTest)
