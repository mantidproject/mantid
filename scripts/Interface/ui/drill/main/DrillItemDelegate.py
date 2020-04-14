# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QItemDelegate
from qtpy.QtGui import QFontMetrics

class DrillItemDelegate(QItemDelegate):
    """
    Class that defines a special delegate for drill table cell contents
    viewing. It allows the reduction of string length when many numors need to
    be displayed in a table cell.
    """
    ELEMENT_SEPARATOR = ','
    NUMORS_SEPARATORS = ['-', '+', ':']

    def __init__(self, parent):
        super(DrillItemDelegate, self).__init__(parent)

    def reduceNumorsStr(self, numorsStr):
        """
        Try to reduce a numors string by putting ellipsis at the end of the
        last reducable element. For example:
        1000+1001,1000,1000       ->  1000...,1000,1000
        1000+1001,1000,1000+1001  ->  1000+1001,1000,1000...
        1000+1001,1000,1000...    ->  1000...,1000,1000...

        Args:
            numorsStr (str): the numors string

        Returns:
            tuple(str, bool): the reduced string and 'True' if the string can
                still be reduced
        """
        reducable = True
        splittedStr = numorsStr.split(self.ELEMENT_SEPARATOR)

        # if there is no ',' in the str
        if len(splittedStr) == 1:
            reducable = False
            return numorsStr, reducable

        # try to reduce the str length, element by element, starting from the end
        reduced = False
        i = len(splittedStr) - 1
        while (not reduced) and (i >= 0):
            for s in self.NUMORS_SEPARATORS:
                numors = splittedStr[i].split(s)
                if len(numors) > 1:
                    splittedStr[i] = numors[0] + "..."
                    reduced = True
            i -=1

        if i < 0:
            reducable = False

        return ','.join(splittedStr), reducable

    def drawDisplay(self, painter, option, rect, text):
        """
        Draw the displayed text. Override of QItemDelegate::drawDisplay.

        Args:
            painter (QPainter): the painter
            option (QStyleOptionViewItem): drawing options
            rect (QRect): cell rectangle
            text (QString): text to display
        """
        fontMetric = QFontMetrics(option.font)

        reducable = True
        boudingRectangle = fontMetric.boundingRect(text)
        while (boudingRectangle.width() > rect.width()) and (reducable):
            text, reducable = self.reduceNumorsStr(text)
            boudingRectangle = fontMetric.boundingRect(text)

        super(DrillItemDelegate, self).drawDisplay(painter, option, rect, text)

