"""
Test the TiledWindow functionality
"""
from mantidplottests import unittest, runTests
from mantidplot import *
import _qti

class MantidPlotTiledWindowTest(unittest.TestCase):

    def test_addWidget(self):

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
        self.assertEqual( tw.rowCount(), 2 )
        self.assertEqual( tw.columnCount(), 2 )
        tw.close()

    def test_removeWidget(self):

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

    def test_selectWidget(self):
        tw = newTiledWindow()
        t1 = newTable()
        t2 = newTable()
        t3 = newTable()
        t4 = newGraph()

        tw.addWidget(t4,1,2)
        tw.addWidget(t1,0,1)
        tw.addWidget(t2,1,0)
        tw.addWidget(t3,2,2)

        tw.selectWidget(0,0)
        self.assertFalse( tw.isSelected(0,0) )
        tw.selectWidget(0,1)
        self.assertTrue( tw.isSelected(0,1) )
        tw.deselectWidget(0,1)
        self.assertFalse( tw.isSelected(0,1) )

        tw.selectWidget(0,1)
        self.assertTrue( tw.isSelected(0,1) )
        tw.selectWidget(1,0)
        self.assertTrue( tw.isSelected(1,0) )
        self.assertFalse( tw.isSelected(0,1) )

        tw.selectRange(1,0,2,2)
        self.assertFalse( tw.isSelected(0,1) )
        self.assertTrue( tw.isSelected(1,0) )
        self.assertTrue( tw.isSelected(1,2) )
        self.assertTrue( tw.isSelected(2,2) )

        tw.deselectWidget(1,2)
        self.assertFalse( tw.isSelected(0,1) )
        self.assertTrue( tw.isSelected(1,0) )
        self.assertFalse( tw.isSelected(1,2) )
        self.assertTrue( tw.isSelected(2,2) )

        tw.close()

    def test_dockSelected(self):
        tw = newTiledWindow()
        t1 = newTable()
        t2 = newTable()
        t3 = newTable()
        t4 = newGraph()

        tw.addWidget(t4,1,2)
        tw.addWidget(t1,0,1)
        tw.addWidget(t2,1,0)
        tw.addWidget(t3,2,2)

        tw.selectRange(0,1,1,0)
        tw.removeSelectionToDocked()

        folder = activeFolder()
        self.assertFalse( folder.findWindow(t1.name()) is None )
        self.assertFalse( folder.findWindow(t2.name()) is None )
        self.assertTrue( folder.findWindow(t3.name()) is None )
        self.assertTrue( folder.findWindow(t4.name()) is None )
        tw.close()
        t1.close()
        t2.close()

    def test_undockSelected(self):
        tw = newTiledWindow()
        t1 = newTable()
        t2 = newTable()
        t3 = newTable()
        t4 = newGraph()

        tw.addWidget(t4,1,2)
        tw.addWidget(t1,0,1)
        tw.addWidget(t2,1,0)
        tw.addWidget(t3,2,2)

        tw.selectRange(0,1,1,0)
        tw.removeSelectionToFloating()

        folder = activeFolder()
        self.assertFalse( folder.findWindow(t1.name()) is None )
        self.assertFalse( folder.findWindow(t2.name()) is None )
        self.assertTrue( folder.findWindow(t3.name()) is None )
        self.assertTrue( folder.findWindow(t4.name()) is None )
        tw.close()
        t1.close()
        t2.close()

    def test_getWidget(self):
        tw = newTiledWindow()
        t1 = newTable()
        t4 = newGraph()

        tw.addWidget(t4,1,2)
        tw.addWidget(t1,0,1)

        tt1 = tw.getWidget(1,2)
        self.assertTrue( isinstance(tt1._getHeldObject(), _qti.Graph) )
        tt4 = tw.getWidget(0,1)
        self.assertTrue( isinstance(tt4._getHeldObject(), _qti.Table) )
        tt0 = tw.getWidget(0,0)
        self.assertTrue( tt0._getHeldObject() is None )
        tw.close()

    def test_clear(self):
        """
        Crashes for some reason.
        Running manually is fine.
        """

        tw = newTiledWindow()
        t1 = newTable()
        t2 = newTable()
        t3 = newTable()
        t4 = newGraph()

        tw.addWidget(t4,1,2)
        tw.addWidget(t1,0,1)
        tw.addWidget(t2,1,0)
        tw.addWidget(t3,2,2)

        tw.clear()

        folder = activeFolder()
        self.assertEqual( len(folder.windows()), 1 )
        self.assertEqual( tw.rowCount(), 1 )
        self.assertEqual( tw.columnCount(), 1 )
        self.assertTrue( tw.getWidget(0,0)._getHeldObject() is None )
        tw.close()

    def test_move_from_one_to_another(self):
        tw1 = newTiledWindow()
        tw1.addWidget( newTable(), 0, 0 )

        tw2 = newTiledWindow()
        w = tw1.getWidget(0,0)
        tw2.addWidget(w,0,0)

        self.assertTrue( tw1.getWidget(0,0)._getHeldObject() is None )
        self.assertFalse( tw2.getWidget(0,0)._getHeldObject() is None )

        tw1.close()
        tw2.close()

    def test_reshape(self):
        tw = newTiledWindow()
        t1 = newTable()
        t2 = newTable()
        t3 = newTable()
        t4 = newGraph()

        tw.addWidget(t4,1,2)
        tw.addWidget(t1,0,1)
        tw.addWidget(t2,1,0)
        tw.addWidget(t3,2,2)

        self.assertEqual( tw.rowCount(), 3 )
        self.assertEqual( tw.columnCount(), 3 )

        tw.reshape(2)
        self.assertEqual( tw.rowCount(), 2 )
        self.assertEqual( tw.columnCount(), 2 )

        tw.reshape(1)
        self.assertEqual( tw.rowCount(), 4 )
        self.assertEqual( tw.columnCount(), 1 )

        tw.reshape(4)
        self.assertEqual( tw.rowCount(), 1 )
        self.assertEqual( tw.columnCount(), 4 )

        tw.reshape(3)
        self.assertEqual( tw.rowCount(), 2 )
        self.assertEqual( tw.columnCount(), 3 )

        tw.reshape(5)
        self.assertEqual( tw.rowCount(), 1 )
        self.assertEqual( tw.columnCount(), 4 )

        tw.close()

    def test_insertWidget(self):
        tw = newTiledWindow()
        t1 = newTable()
        t2 = newTable()
        t3 = newTable()
        t4 = newGraph()

        tw.addWidget(t1,0,0)
        tw.addWidget(t2,0,1)
        tw.addWidget(t3,1,0)
        tw.addWidget(t4,1,1)

        self.assertTrue( isinstance(tw.getWidget(0,1)._getHeldObject(), _qti.Table) )

        t5 = newGraph()
        tw.insertWidget(t5,0,1)
        self.assertEqual( tw.rowCount(), 3 )
        self.assertEqual( tw.columnCount(), 2 )
        self.assertTrue( isinstance(tw.getWidget(0,1)._getHeldObject(), _qti.Graph) )

        t6 = newGraph()
        tw.insertWidget(t6,0,0)
        self.assertEqual( tw.rowCount(), 3 )
        self.assertEqual( tw.columnCount(), 2 )
        self.assertTrue( isinstance(tw.getWidget(0,0)._getHeldObject(), _qti.Graph) )
        self.assertTrue( isinstance(tw.getWidget(0,1)._getHeldObject(), _qti.Table) )

        tw.close()

# Run the unit tests
runTests(MantidPlotTiledWindowTest)
