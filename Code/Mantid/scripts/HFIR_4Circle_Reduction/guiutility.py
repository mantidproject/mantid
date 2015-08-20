#
# GUI Utility Methods
#
from PyQt4 import QtGui


def parse_integers_editors(line_edit_list):
    """
    :param line_edit_list:
    :return: (True, list of integers); (False, error message)
    """
    # Check
    assert isinstance(line_edit_list, list)

    error_message = ''
    integer_list = []
    
    for line_edit in line_edit_list:
        assert isinstance(line_edit, QtGui.QLineEdit)
        try:
            str_value = str(line_edit.text()).strip()
            int_value = int(str_value)
        except ValueError as e:
            error_message += 'Unable to parse to integer. %s\n' % (str(e))
        else:
            if str_value != '%d' % int_value:
                error_message += 'Value %s is not a proper integer.\n' % str_value
            else:
                integer_list.append(int_value)
                print 'Value %s to %d' % (str_value, int_value)
        # END-TRY
    # END-FOR
    
    if len(error_message) > 0:
        return False, error_message
    
    return True, integer_list
