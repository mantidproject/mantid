#!/usr/bin/env python

import sys
import time
import argparse
import os

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
            if not self.mainWindow.running:
                # Don't monitor files while a test suite is building or running
                if self.monitor_tests:
                    if test_info.all_tests.is_source_modified(selected_only=True):
                        self.mainWindow.emit( QtCore.SIGNAL("testMonitorChanged()") )
                
                if self.monitor_libraries:
                    pass
            
            time.sleep(self.delay)
            
        
        
        
                
        
#==================================================================================================
#==================================================================================================
class TestViewerMainWindow(QtGui.QMainWindow, ui_main_window.Ui_MainWindow):    
    """
        The main GUI window of the test viewer.
    """

    def __init__(self, settings=None, bin_folder="", source_folder="", parent=None):      
        QtGui.QWidget.__init__(self, parent)
        
        # Save the settings object
        self.settings = settings
        self.bin_folder = bin_folder
        self.source_folder = source_folder
        
        # Populate all GUI elements
        self.setupUi(self)
        self.buttonTest.hide()
        self.set_running(False)
             
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
        self.connect(self, QtCore.SIGNAL("testRun"), self.test_run_callback)
        # Signal called by the monitor thread
        self.connect(self, QtCore.SIGNAL("testMonitorChanged()"), self.run_selected)
        
        self.last_tree_update = time.time()
        # The accumulated stdoutput from commands
        self.stdout = ""
        
        self.setup_tree()
        
        self.current_results = None
        self.show_results()
        
        self.readSettings()
        
    #-----------------------------------------------------------------------------
    def setup_tree(self):
        """ Set up the QTreeWidget of the tree """
        
        # Re-discover all the projects
        # TODO: Use arguments for the source paths
        test_info.all_tests.discover_CXX_projects(self.bin_folder, self.source_folder)
        
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
            

    #-----------------------------------------------------------------------------
    def readSettings(self):
        s = self.settings
        self.checkInParallel.setChecked( s.value("checkInParallel", False).toBool() )
        self.checkMonitorTests.setChecked( s.value("checkMonitorTests", False).toBool() )
        self.checkMonitorLibraries.setChecked( s.value("checkMonitorLibraries", False).toBool() )
        if s.contains("splitter"): self.splitter.restoreState( s.value("splitter").toByteArray() )
        self.resize( s.value("TestViewerMainWindow.width", 1500).toInt()[0], 
                     s.value("TestViewerMainWindow.height", 900).toInt()[0] )
        self.move( s.value("TestViewerMainWindow.x", 0).toInt()[0], 
                   s.value("TestViewerMainWindow.y", 0).toInt()[0] )
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
        s.setValue("TestViewerMainWindow.x", self.x())
        s.setValue("TestViewerMainWindow.y", self.y())
        
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
    def markup_console(self, in_line):
        """Return a marked-up (with HTML) version of a line
        from the consolue. """
        line = in_line
        if ("Error" in line) or ("error:" in line) \
            or ("terminate called after throwing an instance" in line) \
            or ("Segmentation fault" in line) \
            or ("  what(): " in line) \
            or ("Assertion" in line and " failed." in line) \
            or ("undefined reference" in line) \
            :
            #An error line!
            color = "red"
        elif "warning:" in line or "Warning:" in line:
            #Just a warning
            color = "orange"
        else:
            #Print normally
            color = ""
        # Add the color tag
        if color != "":
            line = '<font color="%s">%s</font>' % (color, line)
        # Add a new line 
        return line + "<br>\n"
        
    #-----------------------------------------------------------------------------
    def test_run_callback(self, text, obj):
        """ Update the progress bar and label when a calculation is done (callback from test runner.
        Parameters:
            obj: either a TestSuite or a TestProject; 
            if it is a string, then it is the stdout of the make command"""

        # Update the tree's data for the suite
        if not obj is None:
            
            # For test results, increment the progress bar
            if isinstance(obj, TestSingle) or isinstance(obj, TestSuite) or isinstance(obj, TestProject):
                val = self.progTest.value()+1 
                self.progTest.setValue( val )
                self.progTest.setFormat("%p% : " + text)
                # Try to update the current results shown
                if not self.current_results is None:
                    if obj.get_fullname() == self.current_results.get_fullname(): 
                        self.show_results()
            
            if isinstance(obj, TestSuite):
                self.model.update_suite(obj)
            elif isinstance(obj, TestProject):
                # Updating the project is now redundant since the test status string is made dynamically
                pass
                #self.model.update_project(obj.name)
            elif isinstance(obj, basestring):
                # String was returned
                if obj == test_info.MSG_ALL_BUILDS_SUCCESSFUL:
                    # Special message that all builds were good
                    # Go back to the test results tab.
                    self.tabWidgetRight.setCurrentIndex(0)
                else:
                    # Accumulated stdout
                    self.stdout += self.markup_console(obj)
                    self.textConsole.setText( self.stdout )
                    sb = self.textConsole.verticalScrollBar();
                    sb.setValue(sb.maximum());
                
                
#        # Every second or so, update the tree too
#        if (time.time() - self.last_tree_update) > 1.0:
#            self.quick_update_tree()
#            self.last_tree_update = time.time()
        
    #-----------------------------------------------------------------------------
    def quick_update_tree(self):
        """ Update the tree view without resetting all the model data """
        self.treeTests.model().update_all()
        
    #-----------------------------------------------------------------------------
    def set_running(self, running):
        """ Sets whether a test suite is running. Adjusts the GUI as needed"""
        self.running=running
        self.buttonAbort.setEnabled(running)
        self.buttonRunAll.setEnabled(not running)
        self.buttonRunSelected.setEnabled(not running)
                
    #-----------------------------------------------------------------------------
    def complete_run(self):
        """ Event called when completing/aborting a test run """
        self.set_running(False)
        
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

                
    #-----------------------------------------------------------------------------
    def copy_filename_to_clipboard(self):
        """Copy the filename in labelFilename to clipboard"""
        import pygtk
        pygtk.require('2.0')
        import gtk
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
        self.set_running(True)
        self.stdout = ""
        self.textConsole.setText("")
        # Select the console output tab (for the make output)        self.running=True

        self.tabWidgetRight.setCurrentIndex(1)
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
       
 
def start():
    # Start the settings object.
    # TODO: Change the company name here and in MantidPlot
    settings = QSettings("ISIS", "MantidTestViewer");
    settings_bin_folder = str(settings.value("bin_folder", "../../Mantid/bin").toString())
    settings_source_folder = str(settings.value("source_folder", "../../Mantid/Framework").toString())
    
    parser = argparse.ArgumentParser(description='GUI to run and view Mantid tests.')
    parser.add_argument('bin_folder', metavar='BINFOLDER', type=str, nargs='?',
                        default="", 
                        help='path to the bin/ folder that contains the test executables. Will use the last path used if not specified:\n%s' % settings_bin_folder)
    parser.add_argument('source_folder', metavar='SOURCEFOLDER', type=str, nargs='?',
                        default="", 
                        help='path to the root of the framework source folder. Will use the last path used if not specified:\n%s' % settings_source_folder)
    
    args = parser.parse_args()
    bin_folder = args.bin_folder
    source_folder = args.source_folder
    
    # No command line arguments? Get them from the settings
    if bin_folder == "":
        bin_folder = settings_bin_folder
        source_folder = settings_source_folder
    else:
        # Use whatever was given.
        settings.setValue("bin_folder", bin_folder)
        if source_folder == "":
            # No source folder given? Use the old one
            if settings.contains("source_folder"):
                source_folder = str(settings.value("source_folder", "../../Mantid/Framework").toString())
            else:
               # Try a relative source folder if not specified
               source_folder = os.path.join( bin_folder, "/../Framework" )
        else:
            # Use whatever was given
            settings.setValue("source_folder", source_folder) 

    print "Starting TestViewer ..."
    print "... bin folder is", bin_folder
    print "... source folder is", source_folder
    print "."
    

    app = QtGui.QApplication([])
    app.setOrganizationName("Mantid")
    app.setOrganizationDomain("mantidproject.org")
    app.setApplicationName("Mantid Test Viewer")
    
    main = TestViewerMainWindow(settings, bin_folder, source_folder)
    main.show()
    
    app.exec_() 
        
if __name__ == '__main__':
    start()
        