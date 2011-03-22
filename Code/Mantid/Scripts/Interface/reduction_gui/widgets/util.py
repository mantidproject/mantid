"""
    Utility functions used be the widgets
"""
CSS_VALID = """QLineEdit {
                background-color: white;
            }"""
            
CSS_INVALID = """QLineEdit {
                background-color: #F7E93A;
            }"""

CSS_DEFAULT = """QLineEdit{}"""

def _check_and_get_float_line_edit(line_edit, min=None):
    """
        Reads the value of a QLineEdit as a double
        and changes the background of the widget 
        according to whether it is valid or not.
        @param line_edit: QLineEdit object
    """
    value = line_edit.text().toDouble()
    if value[1] and (min is None or value[0]>min):
        line_edit.setStyleSheet(CSS_DEFAULT)
    else:
        line_edit.setStyleSheet(CSS_INVALID)
    return value[0]

def _check_and_get_int_line_edit(line_edit):
    """
        Reads the value of a QLineEdit as a double
        and changes the background of the widget 
        according to whether it is valid or not.
        @param line_edit: QLineEdit object
    """
    value = line_edit.text().toInt()
    if value[1]:
        line_edit.setStyleSheet(CSS_DEFAULT)
    else:
        line_edit.setStyleSheet(CSS_INVALID)
    return value[0]