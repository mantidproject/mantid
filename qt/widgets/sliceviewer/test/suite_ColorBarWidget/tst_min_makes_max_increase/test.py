# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

def main():
    startApplication("ColorBarWidgetDemo")
    spinDown(waitForObject(":valMax_QScienceSpinBox"))
    spinDown(waitForObject(":valMax_QScienceSpinBox"))
    waitFor("object.exists(':valMax_QScienceSpinBox')", 20000)
    test.compare(findObject(":valMax_QScienceSpinBox").value, 98)
    doubleClick(waitForObject(":qt_spinbox_lineedit_QLineEdit"), 22, 11, 0, Qt.LeftButton)
    type(waitForObject(":valMax_QScienceSpinBox"), "<Ctrl+A>")
    type(waitForObject(":valMax_QScienceSpinBox"), "1")
    type(waitForObject(":valMax_QScienceSpinBox"), "2")
    waitFor("object.exists(':valMax_QScienceSpinBox')", 20000)
    test.compare(findObject(":valMax_QScienceSpinBox").value, 12)
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    spinUp(waitForObject(":valMin_QScienceSpinBox"))
    waitFor("object.exists(':valMax_QScienceSpinBox')", 20000)
    test.verify(findObject(":valMax_QScienceSpinBox").value > 14)
    sendEvent("QCloseEvent", waitForObject(":_QMainWindow"))

