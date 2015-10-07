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
    scanno = 83
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
    peak_info1.set_hkl_raw_data()

    exp_no_new, scan_no_new, pt_no_new = peak_info1.getExpInfo()
    assert_equals((exp_number, scan_number, pt_number), (exp_no_new, scan_no_new, pt_no_new))
    h, k, l = peak_info1.getHKL()
    assert_equals(h, 2.0)
    assert_equals(k, 2.0)
    assert_equals(l, 2.0)

    # Add peak 2
    exp_number = 355
    scan_number = 83
    pt_number = 11
    added2, peak_info2 = wk_flow.add_peak_info(exp_number, scan_number, pt_number)
    assert_true(added2)
    peak_info2.set_hkl_raw_data()

    exp_no_new, scan_no_new, pt_no_new = peak_info2.getExpInfo()
    assert_equals((exp_number, scan_number, pt_number), (exp_no_new, scan_no_new, pt_no_new))
    h, k, l = peak_info2.getHKL()
    assert_equals(h, 2.0)
    assert_equals(k, -2.0)
    assert_equals(l, 0.0)

    peak_info_list = [peak_info1, peak_info2]

    # Lattice
    a = b = c = 3.9
    alpha = beta = gamma = 90.

    # Calculate UB matrix
    status, ub_matrix = wk_flow.calculate_ub_matrix(peak_info_list, a, b, c, alpha, beta, gamma)
    assert_true(status)

    assert_equals(ub_matrix.shape, (3, 3))

    wk_flow.save_current_ub()

    return


@step(u'Then I get the UB matrix and calculate HKL values for the 2 peaks given earlier')
def check_ub_matrix(step):
    """ Retrive UB matrix and check its value 
    """
    # Work flow
    wk_flow = mydata.getObject()

    # Get UB matrix
    status, ret_obj = wk_flow.get_last_ub()
    assert_true(status)

    ub_matrix = ret_obj[1]

    # Get UB matrix
    assert_equals(ub_matrix.shape, (3,3))
    assert_equals(ub_matrix[0][0], 1.0)
    assert_equals(ub_matrix[2][2], 2.0)

    return

@step(u'Then I used the calculated UB matrix to index some peaks')
def index_peaks_by_ub(step):
    """ Index peaks of scan/pt 38/11 and 83/11 with calculated UB matrix
    :param step:
    :return:
    """
    # Work flow
    wk_flow = mydata.getObject()

    # Get UB matrix
    status, ret_obj = wk_flow.get_last_ub()
    assert_true(status)

    ub_peak_ws = ret_obj[0]

    # Index peak 1
    exp_number = 355
    scan_number = 38
    pt_number = 11
    wk_flow.index_peak(exp_number, scan_number, pt_number, ub_peak_ws)

    return

@step(u'Then I import more peaks to refine UB matrix and lattice parameters')
def refine_ub_matrix(step):
    """ Refine UB matrix
    :param step:
    :return:
    """
    # Get work flow
    wk_flow = mydata.getObject()

    return
