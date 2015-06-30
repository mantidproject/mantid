from lettuce import *
from nose.tools import assert_equals

import sys
import os
import os.path


# Import GUI from upper directory
testdir = os.getcwd()
libdir = os.path.join(testdir, os.pardir)
sys.path.append(libdir)

import reduce4circleControl as r4c


class MyData(object):
    """ 
    """
    def __init__(self):
        """ Init
        """
        self.myObject = None

    def __str__(self):
        """ Nice output
        """
        return str(self.myObject)


    def getObject(self):
        """ Get
        """
        return self.myObject

    def setObject(self, inputobject):
        """ Set
        """
        if inputobject is None:
            raise NotImplementedError("Input object is not supposed to be None.")

        self.myObject = inputobject

        return

mydata = MyData()

@step(u'Given I am using HFIR 4-circle data reduction GUI')
def setUp(step):
    """ Set up
    """
    wkflow = r4c.CWSCDReductionControl('HB3A')
    mydata.setObject(wkflow)

    return


@step(u'Given I input experiment number and select the mode to access data from server directly')
def setExperimentInfo(step):
    """ Set up experiment information
    """
    wkflow = mydata.getObject()

    wkflow.setServerURL('http://neutron.ornl.gov/user_data/hb3a/')
    wkflow.setWebAccessMode('download')
    wkflow.setLocalCache('./temp/')

    wkflow.setExpNumber(355)

    return


@step(u'Then I load one data set, find 1 peak from it and specify its HKL value')
def addPeak1(step):
    """ Add one peak
    """
    scanno = 38
    ptno = 11

    # Download data
    wkflow = mydata.getObject()
    if wkflow.existDataFile(scanno, ptno)[0] is False: 
        wkflow.downloadData(scanno, ptno)


    status, retobj = wkflow.findPeak(scanno, ptno)
    if status is True:
        peakinfo = retobj

    millerindex = (1, 2, 3) 
    wkflow.addPeak(peakinfo, millerindex)

    return


@step(u'Then I load another data set, find 1 peak from it and specify its HKL value')
def addPeak2(step):
    """ Add another peak
    """
    scanno = 82
    ptno = 11
    status, retobj = wkflow.findPeak(scanno, ptno)
    if status is True:
        peakinfo = retobj

    millerindex = (1, 2, 3) 
    wkflow.addPeak(peakinfo, millerindex)

    return

@step(u'Then I calculate UB matrix from the 2 reflections')
def calUBMatrix(step):
    """ Calculate UB matrix
    """
    wkflow.calUBMatrix()

    return


@step(u'Then I get the UB matrix and calculate HKL values for the 2 peaks given earlier')
def checkUBMatrix(step):
    """ Retrive UB matrix and check its value 
    """
    ubmatrix = wkflow.getUBMatrix()

    return
