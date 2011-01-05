#!/usr/bin/env python

import sys
import time

from PyQt4 import QtGui, uic, QtCore
import ui_main_window
import test_info

from QtGui import QTreeWidget


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
        return test_info.run_tests_computation_steps(selected_only, make_tests)
        

    #-----------------------------------------------------------------------------
    def test_run_callback(self, suite):
        """ Simple callback for running tests"""
        if isinstance(suite, test_info.TestSuite):
            text = "%s done." % suite.classname
        else:
            text = suite
        self.mainWindow.emit( QtCore.SIGNAL("testRun"), text)
        
        
    #-----------------------------------------------------------------------------
    def run(self):
        print "Test Run started..."
        test_info.run_tests_in_parallel(self.selected_only, self.make_tests, 
                          self.parallel, callback_func= self.test_run_callback)
        
        
        
        

class TestViewerMainWindow(QtGui.QMainWindow, ui_main_window.Ui_MainWindow):    
    """
        The main GUI window of the test viewer.
    """

    def __init__(self, parent=None, state=None, settings=None):      
        QtGui.QWidget.__init__(self, parent)
        # Populate all GUI elements
        self.setupUi(self)
             
        # Create the worker
        self.worker = TestWorker()
        self.connect(self.worker, QtCore.SIGNAL("finished()"), self.complete_run)
        self.connect(self.worker, QtCore.SIGNAL("terminated()"), self.complete_run)
        
        # --- Menu Commands ---
        self.connect(self.action_Quit, QtCore.SIGNAL("triggered()"), self.quit)
        self.connect(self.buttonRunAll, QtCore.SIGNAL("clicked()"), self.run_all)
        self.connect(self.buttonRunSelected, QtCore.SIGNAL("clicked()"), self.run_selected)
        self.connect(self.buttonAbort, QtCore.SIGNAL("clicked()"), self.abort)
        
        # Signal that will be called by the worker thread
        self.connect(self, QtCore.SIGNAL("testRun"), self.update_label)

    def setup_tree(self):
        """ Set up the QTreeWidget of the tree """
        tree = self.treeTests
        #@type tree QTreeWidget
        tree.addTopLevelItem
        
    #-----------------------------------------------------------------------------
    def update_label(self, text):
        val = self.progTest.value()+1 
        self.progTest.setValue( val )
        self.progTest.setFormat("%p% : " + text)
        
    #-----------------------------------------------------------------------------
    def complete_run(self):
        self.buttonAbort.setEnabled(False)
        self.progTest.setValue(0)
        self.progTest.setFormat("")
        
    #-----------------------------------------------------------------------------
    def quit(self):
        """ Exit the program """
        test_info.abort()
        print "Exiting TestViewer. Happy coding!"
        self.close()
        
    #-----------------------------------------------------------------------------
    def abort(self):
        test_info.abort()
       
            
    #-----------------------------------------------------------------------------
    def run_all(self):
        self.do_run(False)

    def run_selected(self):
        self.do_run(True)
        
    def do_run(self, selected_only):
        parallel = self.checkInParallel.isChecked()
        # Do some setup of the worker and GUI
        num_steps = self.worker.set_parameters(self, selected_only=False, make_tests=True, parallel=parallel)
        if num_steps < 1: num_steps = 1 
        self.progTest.setValue(0)
        self.progTest.setMaximum( num_steps )
        self.buttonAbort.setEnabled(True)
        # Begin the thread in the background
        self.worker.start()
        
        
 
        
def start(argv=[]):
    # Initialize the projects ...
    test_info.discover_projects("/home/8oz/Code/Mantid/Code/Mantid/bin/", "/home/8oz/Code/Mantid/Code/Mantid/Framework/")
    
    app = QtGui.QApplication(argv)
    app.setOrganizationName("Mantid")
    app.setOrganizationDomain("mantidproject.org")
    app.setApplicationName("Mantid Test Viewer")
    
    main = TestViewerMainWindow()    
    main.show()
    app.exec_() 
        
        
if __name__ == '__main__':
    print "Starting TestViewer..."
    start(argv=sys.argv)