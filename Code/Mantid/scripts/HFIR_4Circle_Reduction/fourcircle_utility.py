#pylint: disable=W0633,too-many-branches
__author__ = 'wzz'

import os
import urllib2
import socket


def check_url(url, read_lines=False):
    """ Check whether a URL is valid
    :param url:
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


def get_scans_list(server_url, exp_no, return_list=False):
    """ Get list of scans under one experiment
    :param server_url:
    :param exp_no:
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
                    err_msg =  "Contains non-integer string %s." % value_str
            except ValueError:
                ret_status = False
                err_msg = "String %s is not an integer." % (value_str)
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
                        err_msg = "Contains non-integer string %s." % (value_str)
                except ValueError:
                    ret_status = False
                    err_msg = "String %s is not an integer." % (value_str)
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
