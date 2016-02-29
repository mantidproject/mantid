#pylint: disable=W0633,too-many-branches
import os
import urllib2
import socket
import numpy

__author__ = 'wzz'


def check_url(url, read_lines=False):
    """ Check whether a URL is valid
    :param url:
    :param read_lines
    :return: boolean, error message
    """
    lines = None
    try:
        # Access URL
        url_stream = urllib2.urlopen(url, timeout=2)

        # Read lines
        if read_lines is True:
            lines = url_stream.readlines()
    except urllib2.URLError as url_error:
        url_stream = url_error
    except socket.timeout:
        return False, 'Time out. Try again!'

    # Return result
    if url_stream.code in (200, 401):
        url_good = True
    else:
        url_good = False

    # Close connect
    url_stream.close()

    # Return
    if read_lines is True:
        return url_good, lines
    if url_good is False:
        error_message = 'Unable to access %s.  Check internet access. Code %d' % (url, url_stream.code)
    else:
        error_message = ''

    return url_good, error_message


def get_hb3a_wavelength(m1_motor_pos):
    """ Get HB3A's wavelength according to motor 'm1''s position.
    :param m1_motor_pos:
    :return: wavelength.  None for no mapping
    """
    assert isinstance(m1_motor_pos, float), 'Motor m1\'s position must be float.'

    # hard-coded HB3A m1 position and wavelength mapping
    m1_pos_list = [(-25.870, 1.003),
                   (-39.170, 1.5424)]

    motor_pos_tolerance = 0.2

    for m1_tup in m1_pos_list:
        this_pos = m1_tup[0]
        if abs(m1_motor_pos-this_pos) < motor_pos_tolerance:
            return m1_tup[1]
    # END-FOR

    return None


def get_scans_list(server_url, exp_no, return_list=False):
    """ Get list of scans under one experiment
    :param server_url:
    :param exp_no:
    :param return_list: a flag to control the return value. If true, return a list; otherwise, message string
    :return: message
    """
    if server_url.endswith('/') is False:
        server_url = '%s/' % server_url
    data_dir_url = '%sexp%d/Datafiles' % (server_url, exp_no)

    does_exist, raw_lines = check_url(data_dir_url, read_lines=True)
    if does_exist is False:
        return "Experiment %d's URL %s cannot be found." % (exp_no, data_dir_url)

    # Scan through the index page
    scan_list = []
    header = 'HB3A_exp%04d_scan' % exp_no
    for line in raw_lines:
        if line.count(header) > 0:
            # try to find file HB3A_exp0123_scan6789.dat
            term = line.split(header)[1].split('.dat')[0]
            scan = int(term)
            # check
            if '%04d' % scan == term:
                scan_list.append(scan)
    # END_FOR
    scan_list = sorted(scan_list)
    if return_list is True:
        return scan_list

    message = 'Experiment %d: Scan from %d to %d' % (exp_no, scan_list[0], scan_list[-1])

    return message


def get_scans_list_local_disk(local_dir, exp_no):
    """ Get scans from a specified directory on local disk
    :param local_dir:
    :param exp_no:
    :return:
    """
    scan_list = []

    file_names = os.listdir(local_dir)
    header = 'HB3A_exp%04d_scan' % exp_no
    for name in file_names:
        if name.count(header) > 0:
            scan = int(name.split(header)[1].split('.dat')[0])
            scan_list.append(scan)

    scan_list = sorted(scan_list)

    if len(scan_list) == 0:
        message = 'Experiment %d: No scan can be found.' % exp_no
    else:
        message = 'Experiment %d: Scan from %d to %d ' % (exp_no, scan_list[0], scan_list[-1])
        num_skip_scans = scan_list[-1] - scan_list[0] + 1 - len(scan_list)
        if num_skip_scans > 0:
            message += 'with %d ' % num_skip_scans
        else:
            message += 'without '
        message += 'missing scans.'

    return message


def parse_int_array(int_array_str):
    """ Validate whether the string can be divided into integer strings.
    Allowed: a, b, c-d, e, f
    :param int_array_str:
    :return:
    """
    int_array_str = str(int_array_str)
    if int_array_str == "":
        return True, []

    # Split by ","
    term_level_0 = int_array_str.split(",")
    integer_list = []

    # For each term
    err_msg = ""
    ret_status = True

    for level0_term in term_level_0:
        level0_term = level0_term.strip()

        # split upon dash -
        num_dashes = level0_term.count("-")
        if num_dashes == 0:
            # one integer
            value_str = level0_term
            try:
                int_value = int(value_str)
                if str(int_value) != value_str:
                    ret_status = False
                    err_msg = "Contains non-integer string %s." % value_str
            except ValueError:
                ret_status = False
                err_msg = "String %s is not an integer." % value_str
            else:
                integer_list.append(int_value)

        elif num_dashes == 1:
            # Integer range
            two_terms = level0_term.split("-")
            temp_list = []
            for i in xrange(2):
                value_str = two_terms[i]
                try:
                    int_value = int(value_str)
                    if str(int_value) != value_str:
                        ret_status = False
                        err_msg = "Contains non-integer string %s." % value_str
                except ValueError:
                    ret_status = False
                    err_msg = "String %s is not an integer." % value_str
                else:
                    temp_list.append(int_value)

                # break loop
                if ret_status is False:
                    break
            # END_FOR(i)
            integer_list.extend(range(temp_list[0], temp_list[1]+1))

        else:
            # Undefined situation
            ret_status = False
            err_msg = "Term %s contains more than 1 dash." % level0_term
        # END-IF-ELSE

        # break loop if something is wrong
        if ret_status is False:
            break
    # END-FOR(level0_term)

    # Return with false
    if ret_status is False:
        return False, err_msg

    return True, integer_list


def get_det_xml_file_name(instrument_name, exp_number, scan_number, pt_number):
    """
    Get detector XML file name (from SPICE)
    :param instrument_name:
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    # check
    assert isinstance(instrument_name, str)
    assert isinstance(exp_number, int), 'Experiment number must be an int but not %s.' % str(type(exp_number))
    assert isinstance(scan_number, int), 'Scan number must be an int but not %s.' % str(type(scan_number))
    assert isinstance(pt_number, int), 'Pt number must be an int but not %s.' % str(type(pt_number))

    # get name
    xml_file_name = '%s_exp%d_scan%04d_%04d.xml' % (instrument_name, exp_number,
                                                    scan_number, pt_number)

    return xml_file_name


def get_det_xml_file_url(server_url, instrument_name, exp_number, scan_number, pt_number):
    """ Get the URL to download the detector counts file in XML format
    :param server_url:
    :param instrument_name:
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    assert isinstance(server_url, str) and isinstance(instrument_name, str)
    assert isinstance(exp_number, int) and isinstance(scan_number, int) and isinstance(pt_number, int)

    base_file_name = get_det_xml_file_name(instrument_name, exp_number, scan_number, pt_number)
    file_url = '%s/exp%d/Datafiles/%s' % (server_url, exp_number, base_file_name)

    return file_url


def get_spice_file_name(instrument_name, exp_number, scan_number):
    """
    Get standard HB3A SPICE file name from experiment number and scan number
    :param instrument_name
    :param exp_number:
    :param scan_number:
    :return:
    """
    assert isinstance(instrument_name, str)
    assert isinstance(exp_number, int) and isinstance(scan_number, int)
    file_name = '%s_exp%04d_scan%04d.dat' % (instrument_name, exp_number, scan_number)

    return file_name


def get_spice_file_url(server_url, instrument_name, exp_number, scan_number):
    """ Get the SPICE file's URL from server
    :param server_url:
    :param instrument_name:
    :param exp_number:
    :param scan_number:
    :return:
    """
    assert isinstance(server_url, str) and isinstance(instrument_name, str)
    assert isinstance(exp_number, int) and isinstance(scan_number, int)

    file_url = '%sexp%d/Datafiles/%s_exp%04d_scan%04d.dat' % (server_url, exp_number,
                                                              instrument_name, exp_number, scan_number)

    return file_url


def get_spice_table_name(exp_number, scan_number):
    """ Form the name of the table workspace for SPICE
    :param exp_number:
    :param scan_number:
    :return:
    """
    table_name = 'HB3A_Exp%03d_%04d_SpiceTable' % (exp_number, scan_number)

    return table_name


def get_raw_data_workspace_name(exp_number, scan_number, pt_number):
    """ Form the name of the matrix workspace to which raw pt. XML file is loaded
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'HB3A_exp%d_scan%04d_%04d' % (exp_number, scan_number, pt_number)

    return ws_name


def get_merged_md_name(instrument_name, exp_no, scan_no, pt_list):
    """
    Build the merged scan's MDEventworkspace's name under convention
    Requirements: experiment number and scan number are integer. Pt list is a list of integer
    :param instrument_name:
    :param exp_no:
    :param scan_no:
    :param pt_list:
    :return:
    """
    # check
    assert isinstance(instrument_name, str)
    assert isinstance(exp_no, int) and isinstance(scan_no, int)
    assert isinstance(pt_list, list)
    assert len(pt_list) > 0

    merged_ws_name = '%s_Exp%d_Scan%d_Pt%d_%d_MD' % (instrument_name, exp_no, scan_no,
                                                     pt_list[0], pt_list[-1])

    return merged_ws_name


def get_merged_hkl_md_name(instrument_name, exp_no, scan_no, pt_list):
    """
    Build the merged scan's MDEventworkspace's name under convention
    Requirements: experiment number and scan number are integer. Pt list is a list of integer
    :param instrument_name:
    :param exp_no:
    :param scan_no:
    :param pt_list:
    :return:
    """
    # check
    assert isinstance(instrument_name, str)
    assert isinstance(exp_no, int) and isinstance(scan_no, int)
    assert isinstance(pt_list, list)
    assert len(pt_list) > 0

    merged_ws_name = '%s_Exp%d_Scan%d_Pt%d_%d_HKL_MD' % (instrument_name, exp_no, scan_no,
                                                         pt_list[0], pt_list[-1])

    return merged_ws_name


def get_merge_pt_info_ws_name(exp_no, scan_no):
    """ Create the standard table workspace's name to contain the information to merge Pts. in a scan
    :param exp_no:
    :param scan_no:
    :return:
    """
    ws_name = 'ScanPtInfo_Exp%d_Scan%d' % (exp_no, scan_no)

    return ws_name


def get_peak_ws_name(exp_number, scan_number, pt_number_list):
    """
    Form the name of the peak workspace
    :param exp_number:
    :param scan_number:
    :param pt_number_list:
    :return:
    """
    # check
    assert isinstance(exp_number, int) and isinstance(scan_number, int)
    assert isinstance(pt_number_list, list) and len(pt_number_list) > 0

    ws_name = 'Peak_Exp%d_Scan%d_Pt%d_%d' % (exp_number, scan_number,
                                             pt_number_list[0],
                                             pt_number_list[-1])

    return ws_name


def get_single_pt_md_name(exp_number, scan_number, pt_number):
    """ Form the name of the MDEvnetWorkspace for a single Pt. measurement
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'HB3A_Exp%d_Scan%d_Pt%d_MD' % (exp_number, scan_number, pt_number)

    return ws_name


def load_hb3a_md_data(file_name):
    """ Load an ASCii file containing MDEvents and generated by mantid algorithm ConvertCWSDMDtoHKL()
    :param file_name:
    :return:
    """
    # check
    assert isinstance(file_name, str) and os.path.exists(file_name)

    # parse
    data_file = open(file_name, 'r')
    raw_lines = data_file.readlines()
    data_file.close()

    # construct ND data array
    xyz_points = numpy.zeros((len(raw_lines), 3))
    intensities = numpy.zeros((len(raw_lines), ))

    # parse
    for i in xrange(len(raw_lines)):
        line = raw_lines[i].strip()

        # skip empty line
        if len(line) == 0:
            continue

        # set value
        terms = line.split(',')
        for j in xrange(3):
            xyz_points[i][j] = float(terms[j])
        intensities[i] = float(terms[3])
    # END-FOR

    return xyz_points, intensities


