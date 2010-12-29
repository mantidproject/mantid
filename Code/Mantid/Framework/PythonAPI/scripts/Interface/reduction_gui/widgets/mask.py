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
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import sys
try:
    import numpy
    import matplotlib.pyplot
    HAS_MPL = True
except:
    HAS_MPL = False


class MaskWidget(QWidget):

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
        self.topSpinBox = QSpinBox(self)
        self.topSpinBox.setFixedSize(QSize(70,25))
        self.topSpinBox.setRange(0, maxFlow)
        self.topSpinBox.setValue(leftFlow)
        self.topSpinBox.setSuffix(" px")
        self.topSpinBox.setAlignment(Qt.AlignRight|Qt.AlignVCenter)
        self.connect(self.topSpinBox, SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        # Bottom mask
        self.bottomSpinBox = QSpinBox(self)
        self.bottomSpinBox.setFixedSize(QSize(70,25))
        self.bottomSpinBox.setRange(0, maxFlow)
        self.bottomSpinBox.setValue(leftFlow)
        self.bottomSpinBox.setSuffix(" px")
        self.bottomSpinBox.setAlignment(Qt.AlignRight|Qt.AlignVCenter)
        self.connect(self.bottomSpinBox, SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        self.leftSpinBox = QSpinBox(self)
        self.leftSpinBox.setFixedSize(QSize(70,25))
        self.leftSpinBox.setRange(0, maxFlow)
        self.leftSpinBox.setValue(leftFlow)
        self.leftSpinBox.setSuffix(" px")
        self.leftSpinBox.setAlignment(Qt.AlignRight|Qt.AlignVCenter)
        self.connect(self.leftSpinBox, SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        self.rightSpinBox = QSpinBox(self)
        self.rightSpinBox.setFixedSize(QSize(70,25))
        self.rightSpinBox.setRange(0, maxFlow)
        self.rightSpinBox.setValue(rightFlow)
        self.rightSpinBox.setSuffix(" px")
        self.rightSpinBox.setAlignment(Qt.AlignRight|Qt.AlignVCenter)
        self.connect(self.rightSpinBox, SIGNAL("valueChanged(int)"),
                     self.valueChanged)

        self.setSizePolicy(QSizePolicy(QSizePolicy.Expanding,
                                       QSizePolicy.Expanding))
        self.setMinimumSize(self.minimumSizeHint())
        self.valueChanged()


    def valueChanged(self):
        a = self.leftSpinBox.value()
        b = self.rightSpinBox.value()
        self.emit(SIGNAL("valueChanged"), a, b)
        self.update()


    def values(self):
        return self.leftSpinBox.value(), self.rightSpinBox.value()

    def minimumSizeHint(self):
        return QSize(200,200)


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
 
        painter = QPainter(self)
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
        
        painter.setBrush(QBrush(QColor(255,255,255,255)))
        painter.drawPolygon(
                    QPolygon([ax, ay, bx, by, cx, cy, dx, dy]))

        # Image
        if HAS_MPL:
            if self._background_data is not None:
                try:
                    matplotlib.pyplot.imsave(fname="data_image.png", arr=self._background_data, cmap="jet")
                    target = QRect(0,0,100, 100)
                    image = QImage("data_image.png")
                    image.scaled(side, side, Qt.KeepAspectRatio)
                    painter.drawImage(target, image)
                except:
                    raise RuntimeError, "Could not process 2D image\n  %s" % sys.exc_value
        elif self._background_file is not None:
            try:
                target = QRect(0,0,100, 100)
                # left, top, right, bottom
                # Assume standard HFIR image
                source = QRect(48,29,298,299)
                image = QImage(self._background_file)
                image.scaled(side, side, Qt.KeepAspectRatio)
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
        
        painter.setBrush(QBrush(QColor(200,200,200,255)))
        # Top band
        painter.drawPolygon(
                    QPolygon([0, 0, LogicalSize, 0, LogicalSize, top_pos, 0, top_pos]))
        # Bottom band
        painter.drawPolygon(
                    QPolygon([0, LogicalSize, 0, bottom_pos, LogicalSize, bottom_pos, LogicalSize, LogicalSize]))
        # Right
        painter.drawPolygon(
                    QPolygon([right_pos, 0, LogicalSize, 0, LogicalSize, LogicalSize, right_pos, LogicalSize]))
        # Left
        painter.drawPolygon(
                    QPolygon([0, 0, left_pos, 0, left_pos, LogicalSize, 0, LogicalSize]))
        
        
    
if __name__ == "__main__":
    import sys

    app = QApplication(sys.argv)
    form = MaskWidget()
    form.setWindowTitle("Mask")
    form.move(0, 0)
    form.show()
    form.resize(400, 400)
    app.exec_()

