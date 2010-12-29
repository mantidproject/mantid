# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/hfir_transmission.ui'
#
# Created: Tue Nov 30 15:04:16 2010
#      by: PyQt4 UI code generator 4.7.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_Frame(object):
    def setupUi(self, Frame):
        Frame.setObjectName("Frame")
        Frame.resize(778, 708)
        Frame.setFrameShape(QtGui.QFrame.StyledPanel)
        Frame.setFrameShadow(QtGui.QFrame.Raised)
        self.label = QtGui.QLabel(Frame)
        self.label.setGeometry(QtCore.QRect(40, 40, 101, 17))
        self.label.setObjectName("label")
        self.transmission_edit = QtGui.QLineEdit(Frame)
        self.transmission_edit.setGeometry(QtCore.QRect(160, 30, 113, 27))
        self.transmission_edit.setObjectName("transmission_edit")
        self.label_2 = QtGui.QLabel(Frame)
        self.label_2.setGeometry(QtCore.QRect(280, 40, 62, 17))
        self.label_2.setObjectName("label_2")
        self.dtransmission_edit = QtGui.QLineEdit(Frame)
        self.dtransmission_edit.setGeometry(QtCore.QRect(310, 30, 113, 27))
        self.dtransmission_edit.setObjectName("dtransmission_edit")
        self.calculate_chk = QtGui.QCheckBox(Frame)
        self.calculate_chk.setGeometry(QtCore.QRect(40, 70, 191, 22))
        self.calculate_chk.setObjectName("calculate_chk")
        self.direct_beam_chk = QtGui.QRadioButton(Frame)
        self.direct_beam_chk.setGeometry(QtCore.QRect(80, 100, 109, 22))
        self.direct_beam_chk.setObjectName("direct_beam_chk")
        self.beam_spreader_chk = QtGui.QRadioButton(Frame)
        self.beam_spreader_chk.setGeometry(QtCore.QRect(210, 100, 141, 22))
        self.beam_spreader_chk.setObjectName("beam_spreader_chk")
        self.verticalLayoutWidget = QtGui.QWidget(Frame)
        self.verticalLayoutWidget.setGeometry(QtCore.QRect(30, 130, 671, 251))
        self.verticalLayoutWidget.setObjectName("verticalLayoutWidget")
        self.widget_placeholder = QtGui.QVBoxLayout(self.verticalLayoutWidget)
        self.widget_placeholder.setObjectName("widget_placeholder")

        self.retranslateUi(Frame)
        QtCore.QMetaObject.connectSlotsByName(Frame)

    def retranslateUi(self, Frame):
        Frame.setWindowTitle(QtGui.QApplication.translate("Frame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("Frame", "Transmission:", None, QtGui.QApplication.UnicodeUTF8))
        self.transmission_edit.setToolTip(QtGui.QApplication.translate("Frame", "Sample transmission in %.", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("Frame", "+/-", None, QtGui.QApplication.UnicodeUTF8))
        self.dtransmission_edit.setToolTip(QtGui.QApplication.translate("Frame", "Uncertainty on the sample transmission.", None, QtGui.QApplication.UnicodeUTF8))
        self.calculate_chk.setToolTip(QtGui.QApplication.translate("Frame", "Select to let the reduction software calculate the transmission.", None, QtGui.QApplication.UnicodeUTF8))
        self.calculate_chk.setText(QtGui.QApplication.translate("Frame", "Calculate transmission", None, QtGui.QApplication.UnicodeUTF8))
        self.direct_beam_chk.setToolTip(QtGui.QApplication.translate("Frame", "Select to use the direct beam method for transmission calculation.", None, QtGui.QApplication.UnicodeUTF8))
        self.direct_beam_chk.setText(QtGui.QApplication.translate("Frame", "Direct beam", None, QtGui.QApplication.UnicodeUTF8))
        self.beam_spreader_chk.setToolTip(QtGui.QApplication.translate("Frame", "Select to use the beam spreader (glassy carbon) method for transmission calculation.", None, QtGui.QApplication.UnicodeUTF8))
        self.beam_spreader_chk.setText(QtGui.QApplication.translate("Frame", "Beam spreader", None, QtGui.QApplication.UnicodeUTF8))

