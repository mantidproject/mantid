
def main():
    startApplication("ColorBarWidgetDemo")
    test.compare(findObject(":valMax_QScienceSpinBox").text, "1.00e+02")
    test.compare(findObject(":valMax_QScienceSpinBox").value, 100)
    waitFor("object.exists(':valMin_QScienceSpinBox')", 20000)
    test.compare(findObject(":valMin_QScienceSpinBox").text, "0.00e+00")
    test.compare(findObject(":valMin_QScienceSpinBox").value, 0)

