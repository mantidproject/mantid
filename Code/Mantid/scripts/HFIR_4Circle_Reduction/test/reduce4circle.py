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
def add_peak1(step):
    """ Add one peak
    """
    exp_number = 355
    scanno = 38
    ptno = 11

    # Find peak
    wkflow = mydata.getObject()

    status, err_msg = wkflow.find_peak(exp_number, scanno, ptno)
    # Check result
    assert_true(status)
    has_ws, peak_ws = wkflow.get_ub_peak_ws(exp_number, scanno, ptno)
    assert_true(has_ws)
    assert_equals(peak_ws.rowCount(), 1)

    return


@step(u'Then I load another data set, find 1 peak from it and specify its HKL value')
def add_peak2(step):
    """ Add another peak
    """
    exp_number = 355
    scanno = 82
    ptno = 11

    # Find peak
    wk_flow = mydata.getObject()

    status, err_msg = wk_flow.find_peak(exp_number, scanno, ptno)
    assert_true(status)
    has_peak_ws, peak_ws = wk_flow.get_ub_peak_ws(exp_number, scanno, ptno)
    assert_true(has_peak_ws)
    assert_equals(peak_ws.rowCount(), 1)

    return


@step(u'Then I calculate UB matrix from the 2 reflections')
def calculate_ub_matrix(step):
    """ Calculate UB matrix
    """
    # Work flow
    wk_flow = mydata.getObject()

    # Add peak 1
    exp_number = 355
    scan_number = 38
    pt_number = 11
    added1, peak_info1 = wk_flow.add_peak_info(exp_number, scan_number, pt_number)
    assert_true(added1)

    # Add peak 2
    exp_number = 355
    scan_number = 82
    pt_number = 11
    added2, peak_info2 = wk_flow.add_peak_info(exp_number, scan_number, pt_number)
    assert_true(added2)

    peak_info_list = [peak_info1, peak_info2]

    # Lattice
    a = b = c = 3.9
    alpha = beta = gamma = 90.

    # Calculate UB matrix
    status, ub_matrix = wk_flow.calculate_ub_matrix(peak_info_list, a, b, c, alpha, beta, gamma)
    assert_true(status)

    return


@step(u'Then I get the UB matrix and calculate HKL values for the 2 peaks given earlier')
def check_ub_matrix(step):
    """ Retrive UB matrix and check its value 
    """
    return

    ubmatrix = wkflow.getUBMatrix()

    return
