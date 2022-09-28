# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import math
import unittest
from unittest import mock

from qtpy.QtCore import QModelIndex, Qt, QRect
from qtpy.QtGui import QColor, QStandardItemModel, QPainter
from qtpy.QtWidgets import QStyleOptionViewItem, QStyle
from mantidqt.widgets.workspacedisplay.matrix.delegate import CustomTextElidingDelegate
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class CustomTextElidingDelegateTest(unittest.TestCase):

    def test_custom_eliding_delegate_does_nothing_for_invalid_indices(self):
        delegate = CustomTextElidingDelegate(padding=5)
        painter = mock.MagicMock(spec=QPainter)
        # we cannot mock the style &index as they are passed to a C++ type that expects real types
        style, invalid_index = QStyleOptionViewItem(), QModelIndex()

        delegate.paint(painter, style, invalid_index)

        painter.save.assert_not_called()
        painter.drawText.assert_not_called()
        painter.restore.assert_not_called()

    def test_custom_eliding_delegate_respects_padding(self):
        padding = 3
        delegate = CustomTextElidingDelegate(padding)
        painter = mock.MagicMock(spec=QPainter)
        # we cannot mock the style & index as they are passed to a C++ type that expects real types
        style, model = QStyleOptionViewItem(), QStandardItemModel(1, 1)
        style.rect = QRect(0, 0, 99, 29)  # give a non-zero sized rectangle to paint in
        text = str(math.pi)
        model.setData(model.index(0, 0), text, Qt.DisplayRole)

        delegate.paint(painter, style, model.index(0, 0))

        painter.save.assert_called_once()
        painter.setPen.assert_not_called()
        painter.fillRect.assert_not_called()
        painter.drawText.assert_called_once()
        painter.restore.assert_called_once()

        # first call and second argument is the text that will be painted
        # the exact text depends on the font metric so we have a looser requirement
        # for matching
        drawn_text = painter.drawText.call_args[0][2]
        self.assertTrue(drawn_text.startswith("3.141592"))
        # Qt uses the 'horizontal ellipsis' unicode symbol for ellision.
        # To avoid confusion with it look like 3 separate '.' characters in code
        # we use the unicode codepoint directly
        self.assertTrue(drawn_text.endswith(b"\xe2\x80\xa6".decode("utf-8")))

    def test_custom_eliding_delegate_respects_forgeround_color(self):
        padding = 3
        delegate = CustomTextElidingDelegate(padding)
        painter = mock.MagicMock(spec=QPainter)
        # we cannot mock the style & index as they are passed to a C++ type that expects real types
        style, model = QStyleOptionViewItem(), QStandardItemModel(1, 1)
        style.rect = QRect(0, 0, 99, 29)  # give a non-zero sized rectangle to paint in
        text, foreground = str(math.pi), QColor(10, 10, 10)
        model.setData(model.index(0, 0), text, Qt.DisplayRole)
        model.setData(model.index(0, 0), foreground, Qt.ForegroundRole)

        delegate.paint(painter, style, model.index(0, 0))

        painter.setPen.assert_called_once_with(foreground)
        painter.fillRect.assert_not_called()

    def test_custom_eliding_delegate_respects_background_color(self):
        padding = 3
        delegate = CustomTextElidingDelegate(padding)
        painter = mock.MagicMock(spec=QPainter)
        # we cannot mock the style & index as they are passed to a C++ type that expects real types
        style, model = QStyleOptionViewItem(), QStandardItemModel(1, 1)
        style.rect = QRect(0, 0, 99, 29)  # give a non-zero sized rectangle to paint in
        text, background = str(math.pi), QColor(5, 5, 5)
        model.setData(model.index(0, 0), text, Qt.DisplayRole)
        model.setData(model.index(0, 0), background, Qt.BackgroundRole)

        delegate.paint(painter, style, model.index(0, 0))

        painter.setPen.assert_not_called()
        painter.fillRect.assert_called_once()
        # the background color object seems different but we only care about value equality
        self.assertEqual(background, painter.fillRect.call_args[0][1])

    def test_custom_eliding_delegate_changes_back_and_foreground_color_if_selected(self):
        padding = 3
        delegate = CustomTextElidingDelegate(padding)
        painter = mock.MagicMock(spec=QPainter)
        # we cannot mock the style & index as they are passed to a C++ type that expects real types
        style, model = QStyleOptionViewItem(), QStandardItemModel(1, 1)
        style.rect = QRect(0, 0, 99, 29)  # give a non-zero sized rectangle to paint in
        style.state = QStyle.State_Selected  # set selected
        text, background = str(math.pi), QColor(5, 5, 5)
        model.setData(model.index(0, 0), text, Qt.DisplayRole)
        model.setData(model.index(0, 0), background, Qt.BackgroundRole)

        delegate.paint(painter, style, model.index(0, 0))

        painter.setPen.assert_called_once()
        painter.fillRect.assert_called_once()

        self.assertEqual(style.palette.highlight(), painter.fillRect.call_args[0][1])
        self.assertEqual(QColor("white"), painter.setPen.call_args[0][0])


if __name__ == "__main__":
    unittest.main()
