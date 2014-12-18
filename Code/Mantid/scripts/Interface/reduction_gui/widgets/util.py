"""
    Utility functions used be the widgets
"""
CSS_VALID = """QLineEdit {
                background-color: white;
            }"""

CSS_INVALID = """QLineEdit {
                background-color: #FFFF3C;
            }"""

CSS_EDITED = """QLineEdit {
                background-color: #DAF7F9;
            }"""

CSS_EDITED_RADIO = """QRadioButton {
                color: rgb(0,153,153);
                background-color: transparent;
            }"""

CSS_TINY = """QLabel {
                color: gray;
                font:10px arial,sans-serif;
                vertical-align:bottom;
                padding:1px;
                background-color: transparent;
            }"""

CSS_DEFAULT = """QLineEdit{}"""

def _check_and_get_float_line_edit(line_edit, min=None):
    """
        Reads the value of a QLineEdit as a double
        and changes the background of the widget
        according to whether it is valid or not.
        @param line_edit: QLineEdit object
    """
    value = 0.0
    try:
        value = float(line_edit.text())
        if (min is None or value>min):
            line_edit.setStyleSheet(CSS_DEFAULT)
        else:
            line_edit.setStyleSheet(CSS_INVALID)
    except ValueError:
        line_edit.setStyleSheet(CSS_INVALID)
    return value

def _check_and_get_int_line_edit(line_edit):
    """
        Reads the value of a QLineEdit as an integer
        and changes the background of the widget
        according to whether it is valid or not.
        @param line_edit: QLineEdit object
    """
    value = 0
    try:
        value = int(line_edit.text())
        line_edit.setStyleSheet(CSS_DEFAULT)
    except ValueError:
        line_edit.setStyleSheet(CSS_INVALID)
    return value

def set_valid(line_edit, is_valid):
    if is_valid:
        line_edit.setStyleSheet(CSS_DEFAULT)
    else:
        line_edit.setStyleSheet(CSS_INVALID)

def set_edited(control, is_edited):
    class_name = control.__class__.__name__
    if is_edited:
        if class_name in ["QRadioButton", "QCheckBox"]:
            new_style = CSS_EDITED_RADIO.replace("QRadioButton", class_name)
        else:
            new_style = CSS_EDITED.replace("QLineEdit", class_name)
    else:
        new_style = CSS_DEFAULT.replace("QLineEdit", class_name)
    control.setStyleSheet(new_style)

def set_tiny(control, is_tiny=True):
    class_name = control.__class__.__name__
    if is_tiny:
        new_style = CSS_TINY.replace("QLabel", class_name)
    else:
        new_style = CSS_DEFAULT.replace("QLineEdit", class_name)
    control.setStyleSheet(new_style)



