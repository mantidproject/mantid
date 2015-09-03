from lettuce import *
from nose.tools import assert_equals, assert_true

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

    wkflow.set_server_url('http://neutron.ornl.gov/user_data/hb3a/')
    wkflow.setWebAccessMode('download')
    wkflow.set_local_data_dir('./temp/')

    wkflow.set_exp_number(355)

    return


@step(u'I download data to a specified directory')
def downloadData(step):
    """ Download data
    """
    wkflow = mydata.getObject()

    scanlist = [38, 83, 91]
    wkflow.download_data_set(scanlist, overwrite=False)

    return


@step(u'Then I load one data set, find 1 peak from it and specify its HKL value')
def addPeak1(step):
    """ Add one peak
    """
    exp_number = 355
    scanno = 38
    ptno = 11

    # Find peak
    wkflow = mydata.getObject()

    status, retobj = wkflow.find_peak(exp_number, scanno, ptno)

    # Check result
    assert_true(status)
    peak_ws = retobj
    assert_equals(peak_ws.rowCount(), 1)

    return

# TODO - Continue from here!
@step(u'Then I load another data set, find 1 peak from it and specify its HKL value')
def addPeak2(step):
    """ Add another peak
    """
    exp_number = 355
    scanno = 82
    ptno = 11
    status, retobj = wkflow.find_peak(exp_number, scanno, ptno)
    if status is True:
        peakinfo = retobj

    millerindex = (1, 2, 3) 
    wkflow.addPeak(peakinfo, millerindex)

    return

@step(u'Then I calculate UB matrix from the 2 reflections')
def calUBMatrix(step):
    """ Calculate UB matrix
    """

    # Set HKL to peak workspace
    peakws = mtd['Combined']
    
    scan =38
    pt = 11
    matrixws = mtd['HB3A_exp355_scan%04d_%04d'%(scan, pt)]

    wkflow.addPeakToCalUB(peakws, 0, matrixws)
    
    scan =83
    pt = 11
    matrixws = mtd['HB3A_exp355_scan%04d_%04d'%(scan, pt)]
    
    wkflow.addPeakToCalUB(peakws, 1, matrixws)

    # Calculate UB matrix
    a=3.9
    b=a
    c=a
    alpha=90.
    beta=alpha
    gamma=alpha


    wkflow.CalculateUBMatrix(peakws, a, b, c, alpha, beta, gamm)
            

    return


@step(u'Then I get the UB matrix and calculate HKL values for the 2 peaks given earlier')
def checkUBMatrix(step):
    """ Retrive UB matrix and check its value 
    """
    ubmatrix = wkflow.getUBMatrix()

    return
