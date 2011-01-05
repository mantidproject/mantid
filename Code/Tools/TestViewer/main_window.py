#!/usr/bin/env python

import sys
import time

from PyQt4 import QtGui, uic, QtCore
import ui_main_window
import test_info

#-----------------------------------------------------------------------------
def test_run_print_callback(suite):
    """ Simple callback for running tests"""
    if isinstance(suite, test_info.TestSuite):
        text = "Running %s" % suite.classname
    else:
        text = suite                
    print text, "global"
    

class TestWorker(QtCore.QThread):
    """ Class to run the tests in the background of the GUI"""

    def __init__(self, parent = None):
        QtCore.QThread.__init__(self, parent)
        self.exiting = False
        
    def begin(self, selected_only=False, make_tests=True, parallel=False):
        """Set the parameters of how the tests will be built and run;
        then, start the worker"""
        self.selected_only = selected_only
        self.make_tests = make_tests
        self.parallel = parallel
        self.start()
        
        
    def test_run_print_callback(self,suite):
        """ Simple callback for running tests"""
        if isinstance(suite, test_info.TestSuite):
            text = "Running %s" % suite.classname
        else:
            text = suite                
        print text,"in worker"
        
    #-----------------------------------------------------------------------------
    def run(self):
        print "Worker running"
        test_info.run_tests_in_parallel(self.selected_only, self.make_tests, 
                          self.parallel, callback_func=self.test_run_print_callback)

        
        

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
        self.connect(self.worker, QtCore.SIGNAL("finished()"), self.updateUi)
        self.connect(self.worker, QtCore.SIGNAL("terminated()"), self.updateUi)
        
        # --- Menu Commands ---
        self.connect(self.action_Quit, QtCore.SIGNAL("triggered()"), self.quit)
        self.connect(self.buttonRunAll, QtCore.SIGNAL("clicked()"), self.run_all)
        self.connect(self.buttonRunSelected, QtCore.SIGNAL("clicked()"), self.run_selected)
        
    #-----------------------------------------------------------------------------
    def updateUi(self):
        print "updateUI"
        pass
        
    #-----------------------------------------------------------------------------
    def quit(self):
        """ Exit the program """
        print "Exiting TestViewer. Happy coding!"
        self.close()
        

            
    #-----------------------------------------------------------------------------
    def run_all(self):
        parallel = self.checkInParallel.isChecked()
        self.worker.begin(selected_only=False, make_tests=True, 
                          parallel=parallel)
        

    def run_selected(self):
        test_info.run_tests_in_parallel(selected_only=False, make_tests=False, 
                          parallel=True, callback_func=test_run_print_callback)
 
 
        
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