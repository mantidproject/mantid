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


class MaskWidget(QWidget):

    def __init__(self, leftFlow=0, rightFlow=0, maxFlow=192, parent=None):
        super(MaskWidget, self).__init__(parent)

        self.n_pixels = maxFlow

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
        x = (self.width() - self.topSpinBox.width()) /2.0
        x = self.get_x_offset() + side/2.0 - self.topSpinBox.width() /2.0
        self.topSpinBox.move(x, y)
        
        # Bottom mask
        y = self.get_y_offset()+side-self.topSpinBox.height()/2.0
        self.bottomSpinBox.move(x, y)
        
        # Left
        y = self.get_y_offset()+side/2.0-self.leftSpinBox.height()/2.0
        x = self.get_x_offset()-self.leftSpinBox.width()/2.0
        self.leftSpinBox.move(x, y)
        
        # Right
        x = self.get_x_offset()+side-self.rightSpinBox.width()/2.0
        self.rightSpinBox.move(x, y)
    
    def get_side(self):
        return min(self.width()-2*(self.width()*0.05+self.topSpinBox.width()), 
                   self.height()-2*(self.height()*0.05+self.topSpinBox.height()))

    def get_x_offset(self):
        #return (self.width() - self.get_side()) / 2
        return self.leftSpinBox.width()/2.0+5.0

    def get_y_offset(self):
        #return (self.height() - self.get_side()) / 2
        return self.topSpinBox.height()/2.0+5.0

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
        
        painter.setBrush(QBrush(QColor(200,200,200,255)))
        painter.drawPolygon(
                    QPolygon([ax, ay, bx, by, cx, cy, dx, dy]))
    
        # Mask
        top = self.topSpinBox.value()
        bottom = self.bottomSpinBox.value()
        left = self.leftSpinBox.value()
        right = self.rightSpinBox.value()
        
        ax, ay = left/self.n_pixels*100.0, top/self.n_pixels*100.0
        bx, by = LogicalSize-right/self.n_pixels*100.0, top/self.n_pixels*100.0
        cx, cy = LogicalSize-right/self.n_pixels*100.0, LogicalSize-bottom/self.n_pixels*100.0
        dx, dy = left/self.n_pixels*100.0, LogicalSize-bottom/self.n_pixels*100.0
        
        painter.setBrush(QBrush(QColor(255,255,255,255)))
        painter.drawPolygon(
                    QPolygon([ax, ay, bx, by, cx, cy, dx, dy]))
        
    
if __name__ == "__main__":
    import sys

    app = QApplication(sys.argv)
    form = MaskWidget()
    form.setWindowTitle("Mask")
    form.move(0, 0)
    form.show()
    form.resize(400, 400)
    app.exec_()

