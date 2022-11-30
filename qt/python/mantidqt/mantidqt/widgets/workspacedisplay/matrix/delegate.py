# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Optional

from qtpy.QtCore import Qt, QObject, QModelIndex
from qtpy.QtGui import QPainter, QColor
from qtpy.QtWidgets import (QStyledItemDelegate, QStyleOptionViewItem, QStyle)


class CustomTextElidingDelegate(QStyledItemDelegate):
    """Supports setting a custom padding for text string elision in a table"""

    def __init__(self, padding: int, parent: Optional[QObject] = None) -> None:
        """Initialize an object with a padding and optional parent

        :param padding: An integer specifying the number of characters
                        to display as elision dots at the end
        :param parent: An optional parent object, defaults to None
        """
        super().__init__(parent)
        self._padding = padding

    def paint(self, painter: QPainter, option: QStyleOptionViewItem, index: QModelIndex):
        """Renders the delegate using the given painter and style option for the item specified by index.

        :param painter: A QPainter object to draw
        :param option: Options to describe what should be drawn
        :param index: _description_
        """
        if not index.isValid():
            return

        # Initialize the style options. This is not very Pythonic as it uses C++
        # references under the hood so opt is affected by the second call
        opt = QStyleOptionViewItem(option)
        self.initStyleOption(opt, index)

        # Standard setup, paint, restore operation
        painter.save()
        try:
            painter.setClipRect(opt.rect)
            foreground_colour, background_colour = index.data(Qt.ForegroundRole), index.data(Qt.BackgroundRole)
            state_selected = option.state & QStyle.State_Selected
            if state_selected:
                painter.setPen(QColor("white"))
                painter.fillRect(option.rect, option.palette.highlight())
            else:
                if foreground_colour is not None:
                    painter.setPen(foreground_colour)
                if background_colour is not None:
                    painter.fillRect(option.rect, background_colour)

            padding = self._padding
            opt.rect = option.rect.adjusted(padding, padding, -padding, -padding)
            painter.drawText(opt.rect, int(Qt.AlignLeft | Qt.AlignVCenter),
                             opt.fontMetrics.elidedText(opt.text, Qt.ElideRight, opt.rect.width()))
        finally:
            painter.restore()
