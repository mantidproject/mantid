# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/hfir_output.ui'
#
# Created: Fri Apr  1 10:15:45 2011
#      by: PyQt4 UI code generator 4.7.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_Frame(object):
    def setupUi(self, Frame):
        Frame.setObjectName("Frame")
        Frame.resize(806, 795)
        Frame.setFrameShape(QtGui.QFrame.StyledPanel)
        Frame.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(Frame)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtGui.QLabel(Frame)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.scrollArea = QtGui.QScrollArea(Frame)
        self.scrollArea.setWidgetResizable(True)
        self.scrollArea.setObjectName("scrollArea")
        self.scrollAreaWidgetContents = QtGui.QWidget(self.scrollArea)
        self.scrollAreaWidgetContents.setGeometry(QtCore.QRect(0, 0, 784, 704))
        self.scrollAreaWidgetContents.setObjectName("scrollAreaWidgetContents")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.scrollAreaWidgetContents)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.output_text_edit = QtGui.QTextEdit(self.scrollAreaWidgetContents)
        self.output_text_edit.setObjectName("output_text_edit")
        self.verticalLayout_2.addWidget(self.output_text_edit)
        self.scrollArea.setWidget(self.scrollAreaWidgetContents)
        self.verticalLayout.addWidget(self.scrollArea)
        self.plot_area_layout = QtGui.QHBoxLayout()
        self.plot_area_layout.setSizeConstraint(QtGui.QLayout.SetDefaultConstraint)
        self.plot_area_layout.setContentsMargins(0, 20, -1, 20)
        self.plot_area_layout.setObjectName("plot_area_layout")
        self.verticalLayout.addLayout(self.plot_area_layout)

        self.retranslateUi(Frame)
        QtCore.QMetaObject.connectSlotsByName(Frame)

    def retranslateUi(self, Frame):
        Frame.setWindowTitle(QtGui.QApplication.translate("Frame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("Frame", "Reduction Output", None, QtGui.QApplication.UnicodeUTF8))

