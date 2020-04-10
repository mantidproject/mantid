# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QHeaderView, QPushButton, QStyle, QStyleOptionButton
from qtpy.QtCore import *

class DrillHeaderView(QHeaderView):
    """
    Class that defines a special header for the DRILL interface table. A push
    button is added in each section header to allow the user to fold and unfold
    it.
    """
    BUTTON_TEXT_FOLDED = "+"    # push button text when the section is folded
    BUTTON_TEXT_UNFOLDED = "-"  # push button text when the section is unfolded
    BUTTON_X_SIZE = 12          # push button horizontal size
    BUTTON_Y_SIZE = 12          # push button vertical size
    BUTTON_MARGIN = 5           # margin around the push button

    def __init__(self):
        super(DrillHeaderView, self).__init__(Qt.Horizontal)
        self.buttonsRectangles = dict()
        self.sectionsFolded = dict()  # folded columns indexes
        self.sectionsTexts = dict()   # folded columns text
        self.sectionsSizes = dict()   # folded columns original size
        self.buttonPressed = None     # currently pressed button, used for release

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
        return size + QSize(self.BUTTON_X_SIZE + 2 * self.BUTTON_MARGIN, 0)

    def paintSection(self, painter, rect, logicalIndex):
        """
        Overriding of QTableView::paintSection. This function is called for
        each section drawing.

        Args:
            painter (QPainter): painter
            rect (QRect): section rectangle
            logicalIndex (int): section logical index
        """
        # button shape
        self.buttonsRectangles[logicalIndex] = QRect(0, 0,
                                                    self.BUTTON_X_SIZE,
                                                    self.BUTTON_Y_SIZE)
        self.buttonsRectangles[logicalIndex].moveCenter(rect.center())
        self.buttonsRectangles[logicalIndex].moveRight(
                rect.right() - self.BUTTON_MARGIN)
        option = QStyleOptionButton()
        option.rect = self.buttonsRectangles[logicalIndex]
        option.features = QStyleOptionButton.AutoDefaultButton

        # button state
        if self.isButtonPressed(logicalIndex):
            option.state = QStyle.State_On
        else:
            option.state = QStyle.State_Off

        # section state
        if not self.isSectionFolded(logicalIndex):
            option.text = self.BUTTON_TEXT_UNFOLDED
        else:
            option.text = self.BUTTON_TEXT_FOLDED

        # call QTableView function
        painter.save()
        super(DrillHeaderView, self).paintSection(painter, rect, logicalIndex)
        painter.restore()

        # paint the push button
        self.style().drawControl(QStyle.CE_PushButton, option, painter)

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
            # set the size back
            size = self.sectionsSizes[li]
            self.resizeSection(li, size)
            # and the text
            text = self.sectionsTexts[li]
            self.model().setHeaderData(li, self.orientation(), text)

            self.sectionsFolded[li] = False
        else:
            # save the size
            self.sectionsSizes[li] = self.sectionSize(li)
            self.resizeSection(li, self.BUTTON_X_SIZE + 2 * self.BUTTON_MARGIN)
            # save and hide the text
            text = self.model().headerData(li, self.orientation())
            self.sectionsTexts[li] = text
            self.model().setHeaderData(li, self.orientation(), "")

            self.sectionsFolded[li] = True

