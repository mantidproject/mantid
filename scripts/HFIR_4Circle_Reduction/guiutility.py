#
# GUI Utility Methods
#
from PyQt4 import QtGui


def parse_float_array(array_str):
    """ Parse a string to an array of float
    :param array_str:
    :return: boolean, list of floats/error message
    """
    print array_str
    assert isinstance(array_str, str)
    array_str = array_str.replace(',', ' ')
    array_str = array_str.replace('\n', ' ')
    array_str = array_str.replace('\t ', ' ')
    array_str = array_str.strip()
    print '[DB] After processing: ', array_str

    float_str_list = array_str.split()
    float_list = list()
    for float_str in float_str_list:
        try:
            value = float(float_str)
        except ValueError as value_error:
            return False, 'Unable to parse %s due to %s.' % (float_str, str(value_error))
        else:
            float_list.append(value)
    # END-FOR

    return True, float_list


def parse_integer_list(array_str):
    """ Parse a string to an array of integer separated by ','
    also, the format as 'a-b' is supported too
    :param array_str:
    :return: boolean, list of floats/error message
    """
    assert isinstance(array_str, str)
    array_str = array_str.replace(' ', '')
    array_str = array_str.replace('\n', '')
    array_str = array_str.replace('\t ', '')

    int_str_list = array_str.split(',')
    integer_list = list()
    for int_str in int_str_list:

        try:
            int_value = int(int_str)
            integer_list.append(int_value)
        except ValueError:
            num_dash = int_str.count('-')
            if num_dash == 1:
                terms = int_str.split('-')
                try:
                    start_value = int(terms[0])
                    end_value = int(terms[1])
                except ValueError:
                    raise RuntimeError('Unable to parse %s due to value error' % int_str)
            elif num_dash == 2 and int_str.startswith('-'):
                terms = int_str[1:].split('-')
                try:
                    start_value = int(terms[0])*-1
                    end_value = int(terms[1])
                except ValueError:
                    raise RuntimeError('Unable to parse %s due to value error' % int_str)
            elif num_dash == 3:
                terms = int_str.split('-')
                try:
                    start_value = -1*int(terms[1])
                    end_value = -1*int(terms[3])
                except ValueError:
                    raise RuntimeError('Unable to parse %s due to value error' % int_str)
                except IndexError:
                    raise RuntimeError('Unable to parse %s due to value error' % int_str)
            else:
                raise RuntimeError('Unable to parse %s due to value error' % int_str)

            integer_list.extend(xrange(start_value, end_value+1))
    # END-FOR

    return integer_list


def parse_float_editors(line_edits):
    """
    :param line_edits:
    :return: (True, list of floats); (False, error message)
    """
    # Set flag
    return_single_value = False

    if isinstance(line_edits, QtGui.QLineEdit) is True:
        line_edit_list = [line_edits]
        return_single_value = True
    elif isinstance(line_edits, list) is True:
        line_edit_list = line_edits
    else:
        raise RuntimeError('Input is not LineEdit or list of LineEdit.')

    error_message = ''
    float_list = []

    for line_edit in line_edit_list:
        assert isinstance(line_edit, QtGui.QLineEdit)
        try:
            str_value = str(line_edit.text()).strip()
            float_value = float(str_value)
        except ValueError as value_err:
            error_message += 'Unable to parse to integer. %s\n' % (str(value_err))
        else:
            float_list.append(float_value)
        # END-TRY
    # END-FOR

    if len(error_message) > 0:
        return False, error_message
    elif return_single_value is True:
        return True, float_list[0]

    return True, float_list


def parse_integers_editors(line_edits):
    """
    :param line_edits:
    :return: (True, list of integers); (False, error message)
    """
    # Set flag
    return_single_value = False

    if isinstance(line_edits, QtGui.QLineEdit) is True:
        line_edit_list = [line_edits]
        return_single_value = True
    elif isinstance(line_edits, list) is True:
        line_edit_list = line_edits
    else:
        raise RuntimeError('Input is not LineEdit or list of LineEdit.')

    error_message = ''
    integer_list = []

    for line_edit in line_edit_list:
        assert isinstance(line_edit, QtGui.QLineEdit)
        try:
            str_value = str(line_edit.text()).strip()
            int_value = int(str_value)
        except ValueError as value_err:
            error_message += 'Unable to parse to integer. %s\n' % (str(value_err))
        else:
            if str_value != '%d' % int_value:
                error_message += 'Value %s is not a proper integer.\n' % str_value
            else:
                integer_list.append(int_value)
        # END-TRY
    # END-FOR

    if len(error_message) > 0:
        return False, error_message
    elif return_single_value is True:
        return True, integer_list[0]

    return True, integer_list
