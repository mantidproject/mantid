# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QHeaderView, QPushButton, QStyle, \
                           QStyleOptionButton, QStyleOptionHeader
from qtpy.QtCore import *

class DrillHeaderView(QHeaderView):
    """
    Class that defines a special header for the DRILL interface table. A push
    button is added in each section header to allow the user to fold and unfold
    it.
    """
    BUTTON_TEXT_FOLDED = "+"    # push button text when the section is folded
    BUTTON_TEXT_UNFOLDED = "-"  # push button text when the section is unfolded
    BUTTON_MARGIN = 2           # margin around the push button

    def __init__(self, parent=None):
        super(DrillHeaderView, self).__init__(Qt.Horizontal, parent)
        self.setDefaultAlignment(Qt.AlignLeft)
        self.buttonsRectangles = dict()
        self.sectionsFolded = dict()  # folded columns indexes
        self.sectionsTexts = dict()   # folded columns text
        self.sectionsSizes = dict()   # folded columns original size
        self.buttonPressed = None     # currently pressed button, used for release

        # set the minimum section size
        margin = self.style().pixelMetric(QStyle.PM_FocusFrameHMargin, None, self)
        minSize = self.fontMetrics().width("....") + 2 * margin
        self.setMinimumSectionSize(minSize)

    def reset(self):
        """
        Override QHeaderView::reset. Reset state of the header.
        """
        self.buttonsRectangles = dict()
        self.sectionsFolded = dict()
        self.sectionsTexts = dict()
        self.sectionsSizes = dict()
        self.buttonPressed = None
        super(DrillHeaderView, self).reset()

    def isButtonPressed(self, li):
        """
        Tells if a push button is pressed by providing the logical index of the
        corresponding section.

        Args:
            li (int): logical index of the column that contains the button

        Returns
            bool: 'True' if the button has been pressed.
        """
        return li == self.buttonPressed

    def isSectionFolded(self, li):
        """
        Tells if a section is folded by providing its logical index.

        Args
            li (int): logical index of the section

        Returns:
            bool: 'True' if the section is currently folded
        """
        return (li in self.sectionsFolded) and (self.sectionsFolded[li])

    def mouseOnButton(self, position, li):
        """
        Tells if the mouse is currently on the section push button.

        Args:
            position (QPoint): the mouse position
            li (int): the section logical index
        """
        return (li in self.buttonsRectangles) \
           and (self.buttonsRectangles[li].contains(position))

    def sectionSizeFromContents(self, li):
        """
        Returns the size of a section by taking into account the new push
        button. Override of QTableView::sectionSizeFromContents used to adapt
        the sections size to their contents.

        Args:
            li (int): section logical index

        Returns:
            (QSize): section size
        """
        size = super(DrillHeaderView, self).sectionSizeFromContents(li)
        buttonSize = size.height() - 2 * self.BUTTON_MARGIN
        size.setWidth(size.width() + buttonSize + 2 * self.BUTTON_MARGIN)
        return size

    def paintSection(self, painter, rect, logicalIndex):
        """
        Overriding of QTableView::paintSection. This function is called for
        each section drawing.

        Args:
            painter (QPainter): painter
            rect (QRect): section rectangle
            logicalIndex (int): section logical index
        """
        # button shape, get the size from the header height
        buttonSize = rect.height() - 2 * self.BUTTON_MARGIN
        self.buttonsRectangles[logicalIndex] = QRect(0, 0,
                                                    buttonSize, buttonSize)
        self.buttonsRectangles[logicalIndex].moveCenter(rect.center())
        # if the section is not folded, the button is right aligned
        if not self.isSectionFolded(logicalIndex):
            self.buttonsRectangles[logicalIndex].moveRight(
                    rect.right() - self.BUTTON_MARGIN)
        buttonOption = QStyleOptionButton()
        buttonOption.rect = self.buttonsRectangles[logicalIndex]
        buttonOption.features = QStyleOptionButton.AutoDefaultButton

        # button state
        if self.isButtonPressed(logicalIndex):
            buttonOption.state = QStyle.State_On
        else:
            buttonOption.state = QStyle.State_Off

        # section state
        if not self.isSectionFolded(logicalIndex):
            buttonOption.text = self.BUTTON_TEXT_UNFOLDED
        else:
            buttonOption.text = self.BUTTON_TEXT_FOLDED

        # button background, only if the section is unfolded
        if not self.isSectionFolded(logicalIndex):
            buttonBackOption = QStyleOptionHeader()
            self.initStyleOption(buttonBackOption)
            buttonBackOption.rect = rect
            buttonBackOption.rect.setWidth(
                   buttonSize + 2 * self.BUTTON_MARGIN)
            buttonBackOption.rect.moveCenter(buttonOption.rect.center())

        # call QTableView function
        painter.save()
        super(DrillHeaderView, self).paintSection(painter, rect, logicalIndex)
        painter.restore()

        # paint the push button background, only if the section is unfoled
        if not self.isSectionFolded(logicalIndex):
            painter.save()
            self.style().drawControl(QStyle.CE_Header,
                                     buttonBackOption, painter)
            painter.restore()

        # paint the push button
        self.style().drawControl(QStyle.CE_PushButton, buttonOption, painter)

    def mousePressEvent(self, event):
        """
        Deal with mouse press event. Override of QTableView::mousePressEvent.
        This function change the state of a eventual push button below the
        mouse pointer.

        Args:
            event (QMouseEvent): mouse press envent
        """
        li = self.logicalIndexAt(event.pos())
        if self.mouseOnButton(event.pos(), li):
            self.buttonPressed = li
            self.updateSection(li)
        else:
            super(DrillHeaderView, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        """
        Deal with mouse release event. Override of
        QTableView::mouseReleaseEvent. This function change the state of a
        eventual currently pressed button.

        Args:
            event (QMouseEvent): mouse release event
        """
        if self.buttonPressed is not None:
            li = self.buttonPressed
            self.buttonPressed = None
            self.changeSectionFolding(li)
            self.updateSection(li)
        else:
            super(DrillHeaderView, self).mouseReleaseEvent(event)

    def changeSectionFolding(self, li):
        """
        Change the section folding state. This function is called when a push
        button is relased. It folds the correponding unfolded section and
        unfolded the corresponding folded section. When folding occurs, the
        size and the text of the section are saved. They are reloaded when the
        section is unfolded.

        Args:
            li (int): section logical index
        """
        if self.isSectionFolded(li):
            # set size and resize mode back
            size = self.sectionsSizes[li]
            self.resizeSection(li, size)
            self.setSectionResizeMode(li, QHeaderView.Interactive)
            # and the text
            text = self.sectionsTexts[li]
            self.model().setHeaderData(li, self.orientation(), text)

            self.sectionsFolded[li] = False
        else:
            # save the size, resize the minimum
            self.sectionsSizes[li] = self.sectionSize(li)
            self.resizeSection(li, self.minimumSectionSize())
            self.setSectionResizeMode(li, QHeaderView.Fixed)
            # save and hide the text
            text = self.model().headerData(li, self.orientation())
            self.sectionsTexts[li] = text
            self.model().setHeaderData(li, self.orientation(), "")

            self.sectionsFolded[li] = True

