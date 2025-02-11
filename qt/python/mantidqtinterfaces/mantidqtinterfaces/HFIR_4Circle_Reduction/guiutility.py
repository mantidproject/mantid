# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
# GUI Utility Methods
#
import math
import numpy
import os
from qtpy.QtWidgets import QDialog, QLineEdit, QVBoxLayout, QDialogButtonBox, QLabel, QPlainTextEdit
from qtpy import QtCore


def convert_str_to_matrix(matrix_str, matrix_shape):
    """
    Convert a string to matrix, which is a numpy ndarray
    Requirements:
    1. a string that can be converted to a matrix (2d array)
    2. shape must be a tuple of integer
    example: ub_str, (3, 3)
    :param matrix_str:
    :param matrix_shape:
    :return: numpy.ndarray, len(shape) == 2
    """
    # check
    assert isinstance(matrix_str, str), "Input matrix (string) %s is not a string but of type %s." % (
        str(matrix_str),
        matrix_str.__class__.__name__,
    )
    assert isinstance(matrix_shape, tuple) and len(matrix_shape) == 2

    # split matrix string to 9 elements and check
    matrix_str = matrix_str.replace(",", " ")
    matrix_str = matrix_str.replace("\n", " ")
    matrix_terms = matrix_str.split()
    assert len(matrix_terms) == 9, "Matrix string split into %s with %d terms." % (str(matrix_terms), len(matrix_terms))

    # create matrix/ndarray and check dimension
    assert matrix_shape[0] * matrix_shape[1] == len(matrix_terms)
    matrix = numpy.ndarray(shape=matrix_shape, dtype="float")
    term_index = 0
    for i_row in range(matrix_shape[0]):
        for j_col in range(matrix_shape[1]):
            matrix[i_row][j_col] = matrix_terms[term_index]
            term_index += 1

    return matrix


def import_scans_text_file(file_name):
    """
    import a plain text file containing a list of scans
    :param file_name:
    :return:
    """
    # check inputs
    assert isinstance(file_name, str), "File name {0} must be a string but not of type {1}.".format(file_name, type(file_name))
    if os.path.exists(file_name) is False:
        raise RuntimeError("File {0} does not exist.".format(file_name))

    # import file
    scan_file = open(file_name, "r")
    raw_lines = scan_file.readline()
    scan_file.close()

    # parse
    scans_str = ""
    for raw_line in raw_lines:
        # get a clean line and skip empty line
        line = raw_line.strip()
        if len(line) == 0:
            continue

        # skip comment line
        if line.startswith("#"):
            continue

        # form the string
        scans_str += line
    # END-FOR

    # convert scans (in string) to list of integers
    scan_list = parse_integer_list(scans_str)
    scan_list.sort()

    return scan_list


def map_to_color(data_array, base_color, change_color_flag):
    """Map 1-D data to color list
    :param data_array:
    :param base_color:
    :param change_color_flag:
    :return:
    """

    def convert_value_to_color(base_color, change_color_flag, num_changes, num_steps_color, value):
        """Convert a value to color
        :param base_color:
        :param change_color_flag:
        :param num_changes:
        :param num_steps_color:
        :param value:
        :return:
        """
        assert 0 < num_changes <= 3
        if num_changes == 1:
            step_list = (value, 0, 0)
        elif num_changes == 2:
            step_list = (value / num_steps_color, value % num_steps_color, 0)
        else:
            num_steps_color_sq = num_steps_color * num_steps_color
            d_2 = value / num_steps_color_sq
            r_2 = value % num_steps_color_sq  # r_2 is for residue of d_2
            d_1 = r_2 / num_steps_color
            d_0 = r_2 % num_steps_color
            step_list = (d_2, d_1, d_0)
        # END-IF

        step_list_index = 0

        color_value_list = [None, None, None]

        for i_color in range(3):
            c_flag = change_color_flag[i_color]
            if c_flag:
                # this color will be changed for color map
                c_step = step_list[step_list_index]
                # color value = base value + max_change / step
                color_value = base_color[i_color] + (0.9999 - base_color[i_color]) / num_steps_color * c_step
                color_value = min(1.0 - 1.0e-10, color_value)
                step_list_index += 1
            else:
                # use base color
                color_value = base_color[i_color]
            # ENDIF

            # set
            color_value_list[i_color] = color_value
        # END-FOR

        return color_value_list

    # check
    assert isinstance(data_array, list) or isinstance(data_array, numpy.ndarray)
    assert len(base_color) == 3
    assert len(change_color_flag) == 3

    # create output list
    array_size = len(data_array)
    color_list = [None] * array_size

    # examine color to change
    num_changes = 0
    for change_flag in change_color_flag:
        if change_flag:
            num_changes += 1
    assert num_changes > 0, "No color to change!"

    # find out number of steps per color
    num_steps_color = int(math.pow(float(max(data_array)), 1.0 / num_changes) + 0.5)

    # calculate
    for array_index in range(array_size):
        value = data_array[array_index]
        rgb = convert_value_to_color(base_color, change_color_flag, num_changes, num_steps_color, value)
        color_list[array_index] = rgb
    # END-FOR

    return color_list


def parse_float_array(array_str):
    """Parse a string to an array of float
    :param array_str:
    :return: boolean, list of floats/error message
    """
    assert isinstance(array_str, str), "Input array {0} for parsing must be of type string but not a {1}.".format(
        array_str, type(array_str)
    )
    array_str = array_str.replace(",", " ")
    array_str = array_str.replace("\n", " ")
    array_str = array_str.replace("\t ", " ")
    array_str = array_str.strip()

    float_str_list = array_str.split()
    float_list = list()
    for float_str in float_str_list:
        try:
            value = float(float_str)
        except ValueError as value_error:
            return False, "Unable to parse %s due to %s." % (float_str, str(value_error))
        else:
            float_list.append(value)
    # END-FOR

    return True, float_list


def parse_integer_list(array_str, expected_size=None):
    """Parse a string to an array of integer separated by ','
    also, the format as 'a-b' is supported too
    :exception: RuntimeError
    :param array_str:
    :param expected_size
    :return: list of floats/error message
    """
    # check input type
    assert isinstance(array_str, str), "Input {0} must be a string but not a {1}".format(array_str, type(array_str))

    # remove space, tab and \n
    array_str = array_str.replace(" ", "")
    array_str = array_str.replace("\n", "")
    array_str = array_str.replace("\t ", "")

    int_str_list = array_str.split(",")
    integer_list = list()
    for int_str in int_str_list:
        try:
            int_value = int(int_str)
            integer_list.append(int_value)
        except ValueError:
            num_dash = int_str.count("-")
            if num_dash == 1:
                terms = int_str.split("-")
                try:
                    start_value = int(terms[0])
                    end_value = int(terms[1])
                except ValueError:
                    raise RuntimeError("Unable to parse %s due to value error" % int_str)
            elif num_dash == 2 and int_str.startswith("-"):
                terms = int_str[1:].split("-")
                try:
                    start_value = int(terms[0]) * -1
                    end_value = int(terms[1])
                except ValueError:
                    raise RuntimeError("Unable to parse %s due to value error" % int_str)
            elif num_dash == 3:
                terms = int_str.split("-")
                try:
                    start_value = -1 * int(terms[1])
                    end_value = -1 * int(terms[3])
                except ValueError:
                    raise RuntimeError("Unable to parse %s due to value error" % int_str)
                except IndexError:
                    raise RuntimeError("Unable to parse %s due to value error" % int_str)
            else:
                raise RuntimeError("Unable to parse %s due to value error" % int_str)

            integer_list.extend(range(start_value, end_value + 1))
    # END-FOR

    # check size
    if expected_size is not None and len(integer_list) != expected_size:
        raise RuntimeError("It is required to have {0} integers given in {1}.".format(expected_size, array_str))

    return integer_list


def parse_float_editors(line_edits, allow_blank=False):
    """
    Requirements:
    - line_edits: list of line edits
    Guarantees:
    - if 'allow blank' then use None for the value
    - return a list of float
    :param line_edits: list/line edit
    :param allow_blank: flag to allow blanks
    :return: (True, list of floats); (False, error message).  If blank is allowed, None value is used for the blank
        LineEdit.
    """
    # Set flag
    return_single_value = False

    if isinstance(line_edits, QLineEdit):
        line_edit_list = [line_edits]
        return_single_value = True
    elif isinstance(line_edits, list):
        line_edit_list = line_edits
    else:
        raise RuntimeError("Input is not LineEdit or list of LineEdit.")

    error_message = ""
    float_list = []

    for line_edit in line_edit_list:
        assert isinstance(line_edit, QLineEdit)
        str_value = str(line_edit.text()).strip()
        if len(str_value) == 0 and allow_blank:
            # allow blank and use None
            float_list.append(None)
        else:
            # parse
            try:
                float_value = float(str_value)
            except ValueError as value_err:
                error_message += "Unable to parse %s to float due to %s\n" % (str_value, value_err)
            else:
                float_list.append(float_value)
            # END-TRY
        # END-IF-ELSE
    # END-FOR

    if len(error_message) > 0:
        return False, error_message
    elif return_single_value is True:
        # return single float mode
        return True, float_list[0]
    else:
        # Final check
        assert len(line_edits) == len(float_list), "Number of input line edits %d is not same as number of output floats %d." % (
            len(line_edits),
            len(float_list),
        )

    return True, float_list


def parse_integers_editors(line_edits, allow_blank=False):
    """
    Requirements:
    - line_edits: list of line edits
    Guarantees:
    - if 'allow blank' then use None for the value
    - return a list of integers
    :param line_edits:
    :param allow_blank: flag to allow empty string and return as a None
    :return: (True, list of integers); (False, error message)
    """
    # Set flag
    return_single_value = False

    if isinstance(line_edits, QLineEdit):
        line_edit_list = [line_edits]
        return_single_value = True
    elif isinstance(line_edits, list):
        line_edit_list = line_edits
    else:
        raise RuntimeError("Input is not LineEdit or list of LineEdit.")

    error_message = ""
    integer_list = list()

    for line_edit in line_edit_list:
        assert isinstance(line_edit, QLineEdit)
        str_value = str(line_edit.text()).strip()
        if len(str_value) == 0 and allow_blank:
            # allowed empty string
            integer_list.append(None)
        else:
            # normal case
            try:
                int_value = int(str_value)
            except ValueError as value_err:
                # compose error message
                error_message += "Unable to parse a line edit {0} with value '{1}' to an integer due to {2}".format(
                    line_edit.objectName(), str_value, value_err
                )
            else:
                if str_value != "%d" % int_value:
                    error_message += "Value %s is not a proper integer.\n" % str_value
                else:
                    integer_list.append(int_value)
            # END-TRY
        # END-IF-ELSE
    # END-FOR

    if len(error_message) > 0:
        return False, error_message
    elif return_single_value is True:
        return True, integer_list[0]

    return True, integer_list


class GetValueDialog(QDialog):
    """
    A dialog that gets a single value
    """

    def __init__(self, parent=None, label_name=""):
        """
        :param parent:
        :param label_name
        """
        super(GetValueDialog, self).__init__(parent)

        layout = QVBoxLayout(self)

        # details information
        self.info_line = QPlainTextEdit(self)
        self.info_line.setEnabled(False)
        layout.addWidget(self.info_line)

        # input
        self.label = QLabel(self)
        self.value_edit = QLineEdit(self)
        layout.addWidget(self.label)
        layout.addWidget(self.value_edit)
        # END-IF-ELSE

        # nice widget for editing the date
        # self.datetime = QDateTimeEdit(self)
        # self.datetime.setCalendarPopup(True)
        # self.datetime.setDateTime(QDateTime.currentDateTime())
        # layout.addWidget(self.datetime)

        # OK and Cancel buttons
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, QtCore.Qt.Horizontal, self)

        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        # set some values
        self.setWindowTitle("Get user input")
        self.label.setText(label_name)

        return

    # get current date and time from the dialog
    def get_value(self):
        """get the value in string
        :return:
        """
        return str(self.value_edit.text())

    def set_message_type(self, message_type):
        """

        :param message_type:
        :return:
        """
        if message_type == "error":
            self.value_edit.setStyleSheet("color: rgb(255, 0, 0);")
        else:
            self.value_edit.setStyleSheet("color: blue")

        return

    def set_title(self, title):
        """
        set window/dialog title
        :param title:
        :return:
        """
        self.setWindowTitle(title)

        return

    def show_message(self, message):
        """
        set or show message
        :param message:
        :return:
        """
        self.info_line.setPlainText(message)

        return


# END-DEF-CLASS


class DisplayDialog(QDialog):
    """
    This is a simple dialog display which can be configured by users
    """

    def __init__(self, parent=None, name="Test"):
        """init
        :param parent:
        """
        super(DisplayDialog, self).__init__(parent)

        layout = QVBoxLayout(self)

        # nice widget for editing the date
        self.message_edit = QPlainTextEdit(self)
        self.message_edit.setReadOnly(True)
        layout.addWidget(self.message_edit)

        self.setWindowTitle("Merged Scans Workspace Names")

        # OK and Cancel buttons
        buttons = QDialogButtonBox(QDialogButtonBox.Ok, QtCore.Qt.Horizontal, self)

        buttons.accepted.connect(self.accept)
        layout.addWidget(buttons)

        self.name = name

        return

    def set_name(self, new_name):
        self.name = new_name

    def show_message(self, message):
        """
        show message
        :param message:
        :return:
        """
        self.message_edit.setPlainText(message)

        return


# END-DEF-CLASS


# static method to create the dialog and return (date, time, accepted)
def get_value(parent=None):
    """Get value from a pop-up dialog
    :param parent:
    :return:
    """
    dialog = GetValueDialog(parent)
    result = dialog.exec_()
    value = dialog.get_value()

    return value, result == QDialog.Accepted


def get_value_from_dialog(parent, title, details, label_name="Equation"):
    """
    pop a dialog with user-specified message and take a value (string) from the dialog to return
    :param title:
    :param details:
    :param label_name:
    :return:
    """
    # start dialog
    dialog = GetValueDialog(parent, label_name=label_name)

    # set up title
    dialog.set_title(title)
    dialog.show_message(details)

    # launch and get result
    result = dialog.exec_()
    print("Method get_value_from_dialog: returned result is {}".format(result))
    if result is False:
        return None

    str_value = dialog.get_value()

    return str_value


def show_message(parent=None, message="show message here!", message_type="info"):
    """
    show message
    :param parent:
    :param message:
    :return: True for accepting.  False for rejecting or cancelling
    """
    dialog = DisplayDialog(parent)
    # dialog.set_name('Teset new name')
    dialog.show_message(message)
    dialog.set_message_type(message)
    result = dialog.exec_()
    # Dialog object: dialog still exists after exec_()

    return result
