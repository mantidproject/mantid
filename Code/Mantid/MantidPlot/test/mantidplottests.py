"""
Utility functions for running python test scripts
inside MantidPlot.
"""
import sys
import os
import unittest
import time

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
    runner.run(suite)
