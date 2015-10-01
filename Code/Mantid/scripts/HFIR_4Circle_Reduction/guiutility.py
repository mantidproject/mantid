#
# GUI Utility Methods
#
from PyQt4 import QtGui


def parse_float_editors(line_edits):
    """
    :param line_edit_list:
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
    :param line_edit_list:
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
