#!/usr/bin/env python
# Copyright (c) 2007-8 Qtrac Ltd. All rights reserved.
# This program or module is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 2 of the License, or
# version 3 of the License, or (at your option) any later version. It is
# provided for educational purposes and is distributed in the hope that
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
# the GNU General Public License for more details.

# Modified for Mantid on 9/23/2010

from __future__ import division
from PyQt4 import QtGui, uic, QtCore
import sys
try:
    import numpy
    import matplotlib.pyplot
    HAS_MPL = True
except:
    HAS_MPL = False
    
from reduction_gui.reduction.sans_script_elements import Mask    
from base_widget import BaseWidget
import ui.ui_mask
import util

class MaskWidget(QtGui.QWidget):

    # Flag to set the spin boxes on the frame (may hide image)
    INSIDE_FRAME = 0
    # Flag to set the spin boxes outside the frame 
    OUTSIDE_FRAME = 1
    # Spin box location flag
    _spinbox_location = OUTSIDE_FRAME

    def __init__(self, leftFlow=0, rightFlow=0, maxFlow=192, parent=None):
        super(MaskWidget, self).__init__(parent)

        self.n_pixels = maxFlow
        self._background_file = None
        self._background_data = None

        # Top mask
        self.topSpinBox = QtGui.QSpinBox(self)
        self.topSpinBox.setFixedSize(QtCore.QSize(70,25))
        self.topSpinBox.setRange(0, maxFlow)
        self.topSpinBox.setValue(leftFlow)
        self.topSpinBox.setSuffix(" px")
        self.topSpinBox.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignVCenter)
        self.connect(self.topSpinBox, QtCore.SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        # Bottom mask
        self.bottomSpinBox = QtGui.QSpinBox(self)
        self.bottomSpinBox.setFixedSize(QtCore.QSize(70,25))
        self.bottomSpinBox.setRange(0, maxFlow)
        self.bottomSpinBox.setValue(leftFlow)
        self.bottomSpinBox.setSuffix(" px")
        self.bottomSpinBox.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignVCenter)
        self.connect(self.bottomSpinBox, QtCore.SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        self.leftSpinBox = QtGui.QSpinBox(self)
        self.leftSpinBox.setFixedSize(QtCore.QSize(70,25))
        self.leftSpinBox.setRange(0, maxFlow)
        self.leftSpinBox.setValue(leftFlow)
        self.leftSpinBox.setSuffix(" px")
        self.leftSpinBox.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignVCenter)
        self.connect(self.leftSpinBox, QtCore.SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        self.rightSpinBox = QtGui.QSpinBox(self)
        self.rightSpinBox.setFixedSize(QtCore.QSize(70,25))
        self.rightSpinBox.setRange(0, maxFlow)
        self.rightSpinBox.setValue(rightFlow)
        self.rightSpinBox.setSuffix(" px")
        self.rightSpinBox.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignVCenter)
        self.connect(self.rightSpinBox, QtCore.SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        self.setSizePolicy(QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding,
                                       QtGui.QSizePolicy.Expanding))
        self.setMinimumSize(self.minimumSizeHint())
        self.valueChanged()


    def valueChanged(self):
        a = self.leftSpinBox.value()
        b = self.rightSpinBox.value()
        self.emit(QtCore.SIGNAL("valueChanged"), a, b)
        self.update()


    def values(self):
        return self.leftSpinBox.value(), self.rightSpinBox.value()

    def minimumSizeHint(self):
        return QtCore.QSize(200,200)


    def resizeEvent(self, event=None):
        side = self.get_side()
                
        # Top mask
        y = self.get_y_offset()-self.topSpinBox.height()/2.0
        if self._spinbox_location==MaskWidget.OUTSIDE_FRAME:
            y -= self.topSpinBox.height()/2.0+1.0
        x = (self.width() - self.topSpinBox.width()) /2.0
        x = self.get_x_offset() + side/2.0 - self.topSpinBox.width() /2.0
        self.topSpinBox.move(x, y)
        
        # Bottom mask
        y = self.get_y_offset()+side-self.topSpinBox.height()/2.0
        if self._spinbox_location==MaskWidget.OUTSIDE_FRAME:
            y += self.topSpinBox.height()/2.0+1.0
        self.bottomSpinBox.move(x, y)
        
        # Left
        y = self.get_y_offset()+side/2.0-self.leftSpinBox.height()/2.0
        x = self.get_x_offset()-self.leftSpinBox.width()/2.0
        if self._spinbox_location==MaskWidget.OUTSIDE_FRAME:
            x -= self.topSpinBox.width()/2.0+1.0
        self.leftSpinBox.move(x, y)
        
        # Right
        x = self.get_x_offset()+side-self.rightSpinBox.width()/2.0
        if self._spinbox_location==MaskWidget.OUTSIDE_FRAME:
            x += self.topSpinBox.width()/2.0+1.0
        self.rightSpinBox.move(x, y)
    
    def get_side(self):
        return min(self.width()-2*(self.width()*0.05+self.topSpinBox.width()), 
                   self.height()-2*(self.height()*0.05+self.topSpinBox.height()))

    def get_x_offset(self):
        #return (self.width() - self.get_side()) / 2
        horiz_margin = self.leftSpinBox.width()/2.0+5.0
        if self._spinbox_location==MaskWidget.OUTSIDE_FRAME: 
            horiz_margin += self.leftSpinBox.width()/2.0
        return horiz_margin

    def get_y_offset(self):
        #return (self.height() - self.get_side()) / 2
        top_margin = self.topSpinBox.height()/2.0+5.0
        if self._spinbox_location==MaskWidget.OUTSIDE_FRAME: 
            top_margin += self.topSpinBox.height()/2.0
        return top_margin

    def set_background(self, filename):
        """
            Sets the background image
            @param filename: file path for background image
        """
        self._background_file = filename
        
    def set_background_data(self, array):
        """
            Sets the background data as an array
            @param array: background data array
        """
        self._background_data = array

    def paintEvent(self, event=None):
        LogicalSize = 100.0
 
        painter = QtGui.QPainter(self)
        #painter.setRenderHint(QPainter.Antialiasing)
        
        side = self.get_side()

        painter.setViewport(self.get_x_offset(),
                            self.get_y_offset(), 
                            side, side)
        painter.setWindow(0, 0, LogicalSize, LogicalSize)
        
        # Detector outline
        ax, ay = 0, 0
        bx, by = LogicalSize, 0
        cx, cy = LogicalSize, LogicalSize
        dx, dy = 0, LogicalSize
        
        painter.setBrush(QtGui.QBrush(QtGui.QColor(255,255,255,255)))
        painter.drawPolygon(
                    QtGui.QPolygon([ax, ay, bx, by, cx, cy, dx, dy]))

        # Image
        if HAS_MPL:
            if self._background_data is not None:
                try:
                    matplotlib.pyplot.imsave(fname="data_image.png", arr=self._background_data, cmap="jet")
                    target = QtCore.QRect(0,0,100, 100)
                    image = QtGui.QImage("data_image.png")
                    image.scaled(side, side, QtCore.Qt.KeepAspectRatio)
                    painter.drawImage(target, image)
                except:
                    raise RuntimeError, "Could not process 2D image\n  %s" % sys.exc_value
        elif self._background_file is not None:
            try:
                target = QtCore.QRect(0,0,100, 100)
                # left, top, right, bottom
                # Assume standard HFIR image
                source = QtCore.QRect(48,29,298,299)
                image = QtGui.QImage(self._background_file)
                image.scaled(side, side, QtCore.Qt.KeepAspectRatio)
                painter.drawImage(target, image, source)
            except:            
                raise RuntimeError, "Could not process image file %s\n  %s" % (str(self._background_file), sys.exc_value)
    
        # Mask
        top = self.topSpinBox.value()
        bottom = self.bottomSpinBox.value()
        left = self.leftSpinBox.value()
        right = self.rightSpinBox.value()
        
        top_pos = top/self.n_pixels*100.0
        bottom_pos = LogicalSize-bottom/self.n_pixels*100.0
        left_pos = left/self.n_pixels*100.0
        right_pos = LogicalSize-right/self.n_pixels*100.0
        
        painter.setBrush(QtGui.QBrush(QtGui.QColor(200,200,200,255)))
        # Top band
        painter.drawPolygon(
                    QtGui.QPolygon([0, 0, LogicalSize, 0, LogicalSize, top_pos, 0, top_pos]))
        # Bottom band
        painter.drawPolygon(
                    QtGui.QPolygon([0, LogicalSize, 0, bottom_pos, LogicalSize, bottom_pos, LogicalSize, LogicalSize]))
        # Right
        painter.drawPolygon(
                    QtGui.QPolygon([right_pos, 0, LogicalSize, 0, LogicalSize, LogicalSize, right_pos, LogicalSize]))
        # Left
        painter.drawPolygon(
                    QtGui.QPolygon([0, 0, left_pos, 0, left_pos, LogicalSize, 0, LogicalSize]))
        
class MaskTabWidget(BaseWidget):
    """
        Widget for setting up the detector mask
    """

    ## Widget name
    name = "Mask"      
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(MaskTabWidget, self).__init__(parent, state, settings, data_type) 
             
        class MaskFrame(QtGui.QFrame, ui.ui_mask.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = MaskFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(Mask())      
  
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.x_min_edit.setValidator(QtGui.QIntValidator(self._content.x_min_edit))
        self._content.x_max_edit.setValidator(QtGui.QIntValidator(self._content.x_max_edit))
        self._content.y_min_edit.setValidator(QtGui.QIntValidator(self._content.y_min_edit))
        self._content.y_max_edit.setValidator(QtGui.QIntValidator(self._content.y_max_edit))
        
        self._content.top_edit.setValidator(QtGui.QIntValidator(self._content.top_edit))
        self._content.bottom_edit.setValidator(QtGui.QIntValidator(self._content.bottom_edit))
        self._content.left_edit.setValidator(QtGui.QIntValidator(self._content.left_edit))
        self._content.right_edit.setValidator(QtGui.QIntValidator(self._content.right_edit))
        
        # Connections
        self.connect(self._content.add_rectangle_button, QtCore.SIGNAL("clicked()"), self._add_rectangle)
        self.connect(self._content.remove_button, QtCore.SIGNAL("clicked()"), self._remove_rectangle)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.top_edit.setText(QtCore.QString(str(state.top)))
        self._content.bottom_edit.setText(QtCore.QString(str(state.bottom)))
        self._content.left_edit.setText(QtCore.QString(str(state.left)))
        self._content.right_edit.setText(QtCore.QString(str(state.right)))
            
        self._content.listWidget.clear()
        for item in state.shapes:
            self._append_rectangle(item)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = Mask()
        
        # Edges
        m.top = util._check_and_get_int_line_edit(self._content.top_edit)
        m.bottom = util._check_and_get_int_line_edit(self._content.bottom_edit)
        m.left = util._check_and_get_int_line_edit(self._content.left_edit)
        m.right = util._check_and_get_int_line_edit(self._content.right_edit)
        
        # Rectangles
        for i in range(self._content.listWidget.count()):
            m.shapes.append(self._content.listWidget.item(i).value)
        
        return m
    
    def _add_rectangle(self):
        # Read in the parameters
        x_min = util._check_and_get_int_line_edit(self._content.x_min_edit)
        x_max = util._check_and_get_int_line_edit(self._content.x_max_edit)
        y_min = util._check_and_get_int_line_edit(self._content.y_min_edit)
        y_max = util._check_and_get_int_line_edit(self._content.y_max_edit)
        
        # Check that a rectangle was defined. We don't care whether 
        # the min/max values were inverted
        if (self._content.x_min_edit.hasAcceptableInput() and
            self._content.x_max_edit.hasAcceptableInput() and
            self._content.y_min_edit.hasAcceptableInput() and
            self._content.y_max_edit.hasAcceptableInput()):
            rect = Mask.RectangleMask(x_min, x_max, y_min, y_max)
            self._append_rectangle(rect)
    
    def _remove_rectangle(self):
        selected = self._content.listWidget.selectedItems()
        for item in selected:
            self._content.listWidget.takeItem( self._content.listWidget.row(item) )
    
    def _append_rectangle(self, rect):
        class _ItemWrapper(QtGui.QListWidgetItem):
            def __init__(self, value):
                QtGui.QListWidgetItem.__init__(self, value)
                self.value = rect
        self._content.listWidget.addItem(_ItemWrapper("Rect: %g < x < %g; %g < y < %g" % (rect.x_min, rect.x_max, rect.y_min, rect.y_max)))    
        
    
if __name__ == "__main__":
    import sys

    app = QtGui.QApplication(sys.argv)
    form = MaskWidget()
    form.setWindowTitle("Mask")
    form.move(0, 0)
    form.show()
    form.resize(400, 400)
    app.exec_()

