
def main():
    startApplication("ColorBarWidgetDemo")
    clickButton(waitForObject(":Log_QCheckBox"))
    waitFor("object.exists(':valMin_QScienceSpinBox')", 20000)
    test.compare(findObject(":valMin_QScienceSpinBox").value, 0.001)
    spinDown(waitForObject(":valMax_QScienceSpinBox"))
    waitFor("object.exists(':valMax_QScienceSpinBox')", 20000)
    test.verify( abs(findObject(":valMax_QScienceSpinBox").value-89.1251) < 1e-3)
    spinUp(waitForObject(":valMax_QScienceSpinBox"))
    waitFor("object.exists(':valMax_QScienceSpinBox')", 20000)
    test.compare(findObject(":valMax_QScienceSpinBox").value, 100)

