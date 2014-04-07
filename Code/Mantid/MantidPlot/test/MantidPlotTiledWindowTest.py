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
        tw.addTile(t1,0,0)
        tw.addTile(t2,0,1)
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
        tw.addTile(t1,0,0)
        tw.addTile(t2,0,1)
        self.assertTrue( folder.findWindow(t1.name()) is None )
        self.assertTrue( folder.findWindow(t2.name()) is None )
        tw.removeTileToDocked(0,0)
        self.assertFalse( folder.findWindow(t1.name()) is None )
        tw.removeTileToFloating(0,1)
        self.assertFalse( folder.findWindow(t2.name()) is None )
        
# Run the unit tests
mantidplottests.runTests(MantidPlotTiledWindowTest)
