"""
Utility functions for running python test scripts
inside MantidPlot.
"""
import sys
import os
import unittest
import time
import qti
from PyQt4 import Qt

def screenshot(widget, filename, description):
    """ Take a screenshot of the widget for displaying
    in a html report
    
    @param widget :: QWidget to grab
    @param filename :: Save to this file (no extension!)
    @param description :: Short descriptive text of what the 
            screenshot should look like
    """
    pix = Qt.QPixmap.grabWidget(widget)
    pix.save

def runTests(classname):
    """ Run the test suite in the class.
    Uses the XML runner if the MANTID_SOURCE environment variable was set.
    """
    # Custom code to create and run this single test suite
    suite = unittest.TestSuite()
    suite.addTest( unittest.makeSuite(classname) )
    # Get the XML runner if the environment variable was set
    src = os.getenv('MANTID_SOURCE')
    if src is None:
        runner = unittest.TextTestRunner()
    else:
        sys.path.append( os.path.join(src, "TestingTools/unittest-xml-reporting/src") )
        import xmlrunner
        runner = xmlrunner.XMLTestRunner(output='Testing')
	
    #Run using either runner
    res = runner.run(suite)
    
    # Set Mantid exit code
    if not res.wasSuccessful():
        qti.app.setExitCode(1)
    else:
        qti.app.setExitCode(0)
    
    return res
