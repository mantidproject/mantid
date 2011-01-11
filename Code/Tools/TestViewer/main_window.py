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

import pygtk
pygtk.require('2.0')
import gtk

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
class TestMonitorFilesWorker(QtCore.QThread):
    """ Worker that runs in the background
    and looks for modified files. """

    def __init__(self, mainWindow, parent = None):
        QtCore.QThread.__init__(self, parent)
        self.mainWindow = mainWindow
        self.exiting = False
        self.monitor_tests = False
        self.monitor_libraries = False
        # Hoy many seconds to wait before checking again
        self.delay = 1.0

    def set_monitor_tests(self, state):
        self.monitor_tests = state

    def set_monitor_libraries(self, state):
        self.monitor_libraries = state
        
    #-----------------------------------------------------------------------------
    def run(self):
        print "Beginning to monitor files and/or libraries..."
        
        while not self.exiting:
            if self.monitor_tests:
                if test_info.all_tests.is_source_modified(selected_only=True):
                    self.mainWindow.emit( QtCore.SIGNAL("testMonitorChanged()") )
                pass
            
            if self.monitor_libraries:
                pass
            
            time.sleep(self.delay)
            
        
        
        
                
        
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
        
        # Now the file monitor
        self.monitor_worker = TestMonitorFilesWorker(self)
        self.monitor_worker.start()
        
        self.connect(self.worker, QtCore.SIGNAL("finished()"), self.complete_run)
        self.connect(self.worker, QtCore.SIGNAL("terminated()"), self.complete_run)
        
        # --- Menu Commands ---
        self.connect(self.action_Quit, QtCore.SIGNAL("triggered()"), self.quit)

        # -- Checkboxes toggle  ----
        self.connect(self.checkShowFailedOnly, QtCore.SIGNAL("stateChanged(int)"), self.checked_show_fail_only)
        self.connect(self.checkShowSelected, QtCore.SIGNAL("stateChanged(int)"), self.checked_show_selected_only)
        self.connect(self.checkMonitorLibraries, QtCore.SIGNAL("stateChanged(int)"), self.checked_monitor_libraries)
        self.connect(self.checkMonitorTests, QtCore.SIGNAL("stateChanged(int)"), self.checked_monitor_tests)


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
        self.connect(self.buttonSelectSVN, QtCore.SIGNAL("clicked()"), self.select_svn)
        self.connect(self.buttonCopyFilename, QtCore.SIGNAL("clicked()"), self.copy_filename_to_clipboard)
        
        # Signal that will be called by the worker thread
        self.connect(self, QtCore.SIGNAL("testRun"), self.update_label)
        # Signal called by the monitor thread
        self.connect(self, QtCore.SIGNAL("testMonitorChanged()"), self.run_selected)
        
        self.last_tree_update = time.time()
        
        self.setup_tree()
        
        self.current_results = None
        self.show_results()
        
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
        # It is probably smart to hold a ref. to the proxy object too
        self.proxy = TreeFilterProxyModel()
        self.proxy.setSourceModel(self.model)
        self.proxy.setFilterWildcard("*G*")
        
        # Set the tree to use the SORTING/FILTERING proxy of the real model
        tree.setModel(self.proxy)
        
        tree.setAlternatingRowColors(True)
        tree.header().setResizeMode(0,QHeaderView.Stretch)
        tree.header().setResizeMode(1,QHeaderView.Interactive)
        tree.header().setResizeMode(2,QHeaderView.Interactive)
        
        tree.connect( tree, QtCore.SIGNAL("clicked (QModelIndex)"), self.tree_clicked)
        tree.connect( tree, QtCore.SIGNAL("activated (QModelIndex)"), self.tree_clicked)
        self.connect( tree.selectionModel(), QtCore.SIGNAL("currentChanged  (const QModelIndex &, const QModelIndex &)"), self.tree_selected)

    #-----------------------------------------------------------------------------
    def test(self):
        pass
            

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
        self.checkMonitorTests.setChecked( s.value("checkMonitorTests", False).toBool() )
        self.checkMonitorLibraries.setChecked( s.value("checkMonitorLibraries", False).toBool() )
        if s.contains("splitter"): self.splitter.restoreState( s.value("splitter").toByteArray() )
        self.resize( s.value("TestViewerMainWindow.width", 1500).toInt()[0], 
                     s.value("TestViewerMainWindow.height", 900).toInt()[0] )
        column_default_width = [200, 230, 100]
        for i in [1,2]:    
            self.treeTests.setColumnWidth( i, s.value("treeTests.columnWidth(%d)"%i, column_default_width[i]).toInt()[0] )
            if self.treeTests.columnWidth(i) < 50:
                self.treeTests.setColumnWidth( i, 50)
        
    #-----------------------------------------------------------------------------
    def saveSettings(self):
        s = self.settings
        s.setValue("checkInParallel", self.checkInParallel.isChecked() )
        s.setValue("checkMonitorTests", self.checkMonitorTests.isChecked() )
        s.setValue("checkMonitorLibraries", self.checkMonitorLibraries.isChecked() )
        s.setValue("splitter", self.splitter.saveState())
        for i in [1,2]:    
            s.setValue("treeTests.columnWidth(%d)"%i, self.treeTests.columnWidth(i) )
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
            if isinstance(obj, TestSingle):
                if obj == self.current_results: self.show_results()
            if isinstance(obj, TestSuite):
                self.model.update_suite(obj)
                if obj == self.current_results: self.show_results()
            if isinstance(obj, TestProject):
                self.model.update_project(obj.name)
                if obj == self.current_results: self.show_results()
                
                
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
        self.model.setupModelData()
        self.treeTests.update()
        self.show_results()
        

    #-----------------------------------------------------------------------------
    def tree_clicked(self, index):
        """ A row of the tree was clicked. Show the test results.
        @param index :: QAbstracItemModel index into the tree.
        """
        # Retrieve the item at that index
        # (returns a QVariant that you have to bring back to a python object)
        item = self.treeTests.model().data( index, Qt.UserRole).toPyObject()
        self.current_results = item
        self.show_results()
        
    #-----------------------------------------------------------------------------
    def tree_selected(self, current_index, previous_index):
        """ Selection in the tree changed from previous_index to current_index """
        item = self.treeTests.model().data( current_index, Qt.UserRole).toPyObject()
        self.current_results = item
        self.show_results()
        

    #-----------------------------------------------------------------------------
    def show_results(self):
        """ Show the test results.
        @param res :: either TestProject, TestSuite, or TestSingle object containing the results to show."""
        #print "show_results"
        if self.current_results is None:
            self.labelTestType.setText("Test Project Results:")
            self.labelFilename.setText( "" ) 
            self.labelTestName.setText( "" ) 
            self.textResults.setText( "" )
            return
        res = self.current_results
        if isinstance(res, TestProject):
            self.labelTestType.setText("Test Project Results:")
            self.labelFilename.setText( "" ) 
        elif isinstance(res, TestSuite):
            self.labelTestType.setText("Test Suite Results:") 
            self.labelFilename.setText( res.source_file ) 
        elif isinstance(res, TestSingle):
            self.labelTestType.setText("Single Test Results:") 
            self.labelFilename.setText( res.parent.source_file ) 
        else:
            raise "Incorrect object passed to show_results; should be TestProject, TestSuite, or TestSingle."
        self.labelTestName.setText( res.get_fullname() ) 
        self.textResults.setText(res.get_results_text() )
                
    def copy_filename_to_clipboard(self):
        """Copy the filename in labelFilename to clipboard"""
        # get the clipboard
        clipboard = gtk.clipboard_get()
        # set the clipboard text data
        clipboard.set_text(str(self.labelFilename.text()) )
        # make our data available to other applications
        clipboard.store()

                
                
    #-----------------------------------------------------------------------------
    def checked_show_fail_only(self, state):
        """ Toggle the filtering """
        self.proxy.set_filter_failed_only(state > 0)
        
    def checked_show_selected_only(self, state):
        """ Toggle the filtering """
        self.proxy.set_filter_selected_only(state > 0)
        
    def checked_monitor_libraries(self, state):
        self.monitor_worker.set_monitor_libraries( state > 0 )
        
    def checked_monitor_tests(self, state):
        self.monitor_worker.set_monitor_tests( state > 0 )
            
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
        
    def select_svn(self):
        """ Select all files modified by SVN st """
        test_info.all_tests.select_svn()
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