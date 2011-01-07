#!/usr/bin/env python

import sys
import time

from PyQt4 import QtGui, uic, QtCore
import PyQt4.QtCore
from PyQt4.QtCore import *
import PyQt4.QtGui
from PyQt4.QtGui import *

import ui_main_window
import test_info
from test_info import TestSuite, TestSingle, TestProject, MultipleProjects

import test_tree
from test_tree import TestTreeModel, TreeItemSuite, TreeItemProject, TreeFilterProxyModel


#==================================================================================================
#==================================================================================================
class TestWorker(QtCore.QThread):
    """ Class to run the tests in the background of the GUI"""

    def __init__(self, parent = None):
        QtCore.QThread.__init__(self, parent)
        self.exiting = False
        
    #-----------------------------------------------------------------------------
    def set_parameters(self, mainWindow, selected_only=False, make_tests=True, parallel=False):
        """Set the parameters of how the tests will be built and run;
        Returns: the number of steps in the runner.
        """
        self.selected_only = selected_only
        self.make_tests = make_tests
        self.parallel = parallel
        self.mainWindow = mainWindow
        return test_info.all_tests.run_tests_computation_steps(selected_only, make_tests)
        

    #-----------------------------------------------------------------------------
    def test_run_callback(self, obj):
        """ Simple callback for running tests. This is called into the MainProcess.
        
        Parameters:
            obj :: the object, either a TestSuite or TestProject that was just calculated
        """
        if isinstance(obj, TestSuite):
            suite = obj
            text = "%s done." % suite.classname
            test_info.all_tests.replace_suite(suite)
            
        elif isinstance(obj, TestProject):
            pj = obj
            # Replace the project in THIS thread!
            test_info.all_tests.replace_project( pj )
            text = "Made project %s" % pj.name
            
        else:
            text = str(obj)                
                    
        self.mainWindow.emit( QtCore.SIGNAL("testRun"), text, obj)

    #-----------------------------------------------------------------------------
    def run(self):
        print "Test Run started..."
        test_info.all_tests.run_tests_in_parallel(self.selected_only, self.make_tests, 
                          self.parallel, callback_func=self.test_run_callback)
        
        
        
        
#==================================================================================================
#==================================================================================================
class TestViewerMainWindow(QtGui.QMainWindow, ui_main_window.Ui_MainWindow):    
    """
        The main GUI window of the test viewer.
    """

    def __init__(self, parent=None, state=None, settings=None):      
        QtGui.QWidget.__init__(self, parent)
        
        # Save the settings object
        self.settings = settings
        
        # Populate all GUI elements
        self.setupUi(self)
             
        # Create the worker
        self.worker = TestWorker()
        self.connect(self.worker, QtCore.SIGNAL("finished()"), self.complete_run)
        self.connect(self.worker, QtCore.SIGNAL("terminated()"), self.complete_run)
        
        # --- Menu Commands ---
        self.connect(self.action_Quit, QtCore.SIGNAL("triggered()"), self.quit)

        # -- Checkboxes toggle  ----
        self.connect(self.checkShowFailedOnly, QtCore.SIGNAL("stateChanged(int)"), self.checked_show_fail_only)
        self.connect(self.checkShowSelected, QtCore.SIGNAL("stateChanged(int)"), self.checked_show_selected_only)


        # -- Button commands ----
        self.connect(self.buttonRunAll, QtCore.SIGNAL("clicked()"), self.run_all)
        self.connect(self.buttonRunSelected, QtCore.SIGNAL("clicked()"), self.run_selected)
        self.connect(self.buttonAbort, QtCore.SIGNAL("clicked()"), self.abort)
        self.connect(self.buttonRefresh, QtCore.SIGNAL("clicked()"), self.setup_tree) #self.update_tree)
        self.connect(self.buttonExpandProjects, QtCore.SIGNAL("clicked()"), self.expand_tree_to_projects)
        self.connect(self.buttonExpandSuites, QtCore.SIGNAL("clicked()"), self.expand_tree_to_suites)
        self.connect(self.buttonExpandAll, QtCore.SIGNAL("clicked()"), self.expand_tree_to_all)
        self.connect(self.buttonTest, QtCore.SIGNAL("clicked()"), self.test)
        self.connect(self.buttonSelectAll, QtCore.SIGNAL("clicked()"), self.select_all)
        self.connect(self.buttonSelectNone, QtCore.SIGNAL("clicked()"), self.select_none)
        self.connect(self.buttonSelectFailed, QtCore.SIGNAL("clicked()"), self.select_failed)
        
        # Signal that will be called by the worker thread
        self.connect(self, QtCore.SIGNAL("testRun"), self.update_label)
        
        self.last_tree_update = time.time()
        
        self.setup_tree()
        
        self.readSettings()
        
    #-----------------------------------------------------------------------------
    def setup_tree(self):
        """ Set up the QTreeWidget of the tree """
        
        # Re-discover all the projects
        # TODO: Use arguments for the source paths
        test_info.all_tests.discover_CXX_projects("/home/8oz/Code/Mantid/Code/Mantid/bin/", "/home/8oz/Code/Mantid/Code/Mantid/Framework/")
        
        tree = self.treeTests
        #@type tree QTreeWidget
        
        # Create the tree model and put it in there 
        # It is important to save the model in self otherwise I think it gets garbage collected without error!
        self.model = TestTreeModel()
        
        self.proxy = TreeFilterProxyModel()
        self.proxy.setSourceModel(self.model)
        self.proxy.setFilterWildcard("*G*")
        tree.setModel(self.proxy)
        
        tree.setAlternatingRowColors(True)
        tree.header().setResizeMode(0,QHeaderView.Stretch)
        tree.setColumnWidth(1,230)
        #tree.header().setResizeMode(1,QHeaderView.ResizeMode)
        #tree.header().setColumnWidth(1, 100)
        #tree.header().setResizeMode(2,QHeaderView.ResizeMode)
        
        tree.connect( tree, QtCore.SIGNAL("clicked (QModelIndex)"), self.tree_clicked)

    def test(self):
        dialog = QtGui.QDialog()
        dialog.setMinimumSize(700,750)
        layout = QtGui.QVBoxLayout(dialog)
        tv = QtGui.QTreeView(dialog)
        tv.setAlternatingRowColors(True)
        layout.addWidget(tv)
        tv.setModel(self.proxy)
        dialog.exec_()
        print "Done!"

#    #-----------------------------------------------------------------------------
#    def filter_tree(self):
#        """ Apply current filtering options to the tree, by hiding rows """
#        failed_only = True
#        
#        mod = self.treeTests.model()
#        tree = self.treeTests
#        for i in xrange(mod.rootItem.childCount()):
#            pj_index = mod.index(i, 0, QModelIndex() )
#            pj_item = mod.rootItem.child(i)
#            print "Hiding ", pj_index.row()
#            tree.setRowHidden( 0, pj_index, True)
#        
#        
        
    #-----------------------------------------------------------------------------
    def readSettings(self):
        s = self.settings
        self.checkInParallel.setChecked( s.value("checkInParallel", False).toBool() )
        if s.contains("splitter"): self.splitter.restoreState( s.value("splitter").toByteArray() )
        self.resize( s.value("TestViewerMainWindow.width", 1500).toInt()[0], 
                     s.value("TestViewerMainWindow.height", 900).toInt()[0] )    
        self.treeTests.setColumnWidth( 1, s.value("treeTests.columnWidth(1)", 230).toInt()[0] )
        self.treeTests.setColumnWidth( 2, s.value("treeTests.columnWidth(2)", 100).toInt()[0] )
        
    #-----------------------------------------------------------------------------
    def saveSettings(self):
        s = self.settings
        s.setValue("checkInParallel", self.checkInParallel.isChecked() )
        s.setValue("splitter", self.splitter.saveState())
        s.setValue("treeTests.columnWidth(1)", self.treeTests.columnWidth(1) )
        s.setValue("treeTests.columnWidth(2)", self.treeTests.columnWidth(2) )
        s.setValue("TestViewerMainWindow.width", self.width())
        s.setValue("TestViewerMainWindow.height", self.height())
        
    #-----------------------------------------------------------------------------
    def closeEvent(self, event):
        """Window is closing """
        # Save all settings that haven't been saved before
        self.saveSettings()
        
    #-----------------------------------------------------------------------------
    def quit(self):
        """ Exit the program """
        test_info.all_tests.abort()
        print "Exiting TestViewer. Happy coding!"
        self.close()
        
        
        
    #-----------------------------------------------------------------------------
    def expand_tree_to_projects(self):
        """ Collapse suites so that only the top projects are shown """
        # Number of root projects
        rowcount = self.treeTests.model().rowCount( QModelIndex() )
        for i in xrange(rowcount):
            # Collapse each project-level one
            self.treeTests.setExpanded( self.treeTests.model().index(i, 0, QModelIndex() ), False)
        
    #-----------------------------------------------------------------------------
    def expand_tree_to_suites(self):
        """ Collapse single test results but expand projects """
        # Number of root projects
        rowcount = self.treeTests.model().rowCount( QModelIndex() )
        for i in xrange(rowcount):
            # Expand each project-level one
            indx = self.treeTests.model().index(i, 0, QModelIndex() )
            self.treeTests.setExpanded( indx, True)
            # And go through the suites and collapse those
            num_suites = self.treeTests.model().rowCount( indx)
            for j in xrange(num_suites):
                suite_indx = self.treeTests.model().index(j, 0, indx )
                self.treeTests.setExpanded( suite_indx, False)
        
    #-----------------------------------------------------------------------------
    def expand_tree_to_all(self):
        """ Expand all tree items """
        self.treeTests.expandAll()
        
        
    #-----------------------------------------------------------------------------
    def update_label(self, text, obj):
        """ Update the progress bar and label.
        Parameters:
            obj: either a TestSuite or a TestProject """
        val = self.progTest.value()+1 
        self.progTest.setValue( val )
        self.progTest.setFormat("%p% : " + text)

        # Update the tree's data for the suite
        if not obj is None:
            if isinstance(obj, TestSuite):
                self.model.update_suite(obj)
            if isinstance(obj, TestProject):
                self.model.update_project(obj.name)
                
#        # Every second or so, update the tree too
#        if (time.time() - self.last_tree_update) > 1.0:
#            self.quick_update_tree()
#            self.last_tree_update = time.time()
        
    #-----------------------------------------------------------------------------
    def quick_update_tree(self):
        """ Update the tree view without resetting all the model data """
        self.treeTests.model().update_all()
        
    #-----------------------------------------------------------------------------
    def complete_run(self):
        """ Event called when completing/aborting a test run """
        self.buttonAbort.setEnabled(False)
        self.buttonRunAll.setEnabled(True)
        self.buttonRunSelected.setEnabled(True)
        self.progTest.setValue(0)
        self.progTest.setFormat("")
        self.update_tree()
        
        failed = test_info.all_tests.failed
        num_run = test_info.all_tests.num_run
        if failed > 0:
            self.labelResult.setStyleSheet("QLabel { background-color: red }")
            self.labelResult.setText("FAILURE! %d of %d tests failed." % (failed, num_run))
        else:
            self.labelResult.setStyleSheet("QLabel { background-color: green }")
            self.labelResult.setText("All Passed! %d of %d tests failed." % (failed, num_run))
        
    #-----------------------------------------------------------------------------
    def update_tree(self):
        """ Update the tree view with whatever the current results are """
        print "update_tree"
        self.model.setupModelData()
        self.treeTests.update()
        

    #-----------------------------------------------------------------------------
    def tree_clicked(self, index):
        """ A row of the tree was clicked. 
        Show the test results. """
        print "Clicked index ", index
        # Retrieve the item at that index
        item = self.treeTests.model().data( index, Qt.UserRole)
        print item
        
                
                
    #-----------------------------------------------------------------------------
    def checked_show_fail_only(self, state):
        """ Toggle the filtering """
        self.proxy.set_filter_failed_only(state > 0)
        
    def checked_show_selected_only(self, state):
        """ Toggle the filtering """
        self.proxy.set_filter_selected_only(state > 0)
            
    #-----------------------------------------------------------------------------
    def run_all(self):
        self.do_run(False)

    def run_selected(self):
        self.do_run(True)
        
    def do_run(self, selected_only):
        """ Start a parallelized test running with the given parameters """
        parallel = self.checkInParallel.isChecked()
        # Do some setup of the worker and GUI
        num_steps = self.worker.set_parameters(self, selected_only=selected_only, make_tests=True, parallel=parallel)
        if num_steps < 1: num_steps = 1 
        self.progTest.setValue(0)
        self.progTest.setMaximum( num_steps )
        self.buttonAbort.setEnabled(True)
        self.buttonRunAll.setEnabled(False)
        self.buttonRunSelected.setEnabled(False)
        # Begin the thread in the background
        self.worker.start()
        
    #-----------------------------------------------------------------------------
    def abort(self):
        test_info.all_tests.abort()
        
    #-----------------------------------------------------------------------------
    def select_all(self):
        """ Select all tests """
        test_info.all_tests.select_all(True)
        self.proxy.invalidateFilter()
        self.treeTests.update()
        
    def select_none(self):
        """ De-Select all tests """
        test_info.all_tests.select_all(False)
        self.proxy.invalidateFilter()
        self.treeTests.update()
        
    def select_failed(self):
        """ Select all failing tests """
        test_info.all_tests.select_failed()
        self.proxy.invalidateFilter()
        self.treeTests.update()
       
 
        
def start(argv=[]):
    
    # Start the settings object.
    # TODO: Change the company name here and in MantidPlot
    settings = QSettings("ISIS", "MantidTestViewer"); 
    
    # Initialize the projects ...
    #test_info.all_tests.make_fake_results()


    app = QtGui.QApplication(argv)
    app.setOrganizationName("Mantid")
    app.setOrganizationDomain("mantidproject.org")
    app.setApplicationName("Mantid Test Viewer")
    
    main = TestViewerMainWindow(settings=settings)
    main.show()
    
    app.exec_() 
        
        
if __name__ == '__main__':
    print "Starting TestViewer..."
    start(argv=sys.argv)