source(findFile("scripts", "global_checks.py"))

def check_mode_buttons(std, ms, ts, sp):
    waitFor("object.exists(':standardButton_QPushButton')", 20000)
    test.compare(fix_bool(findObject(":standardButton_QPushButton").enabled), std)
    waitFor("object.exists(':multiSliceButton_QPushButton')", 20000)
    test.compare(fix_bool(findObject(":multiSliceButton_QPushButton").enabled), ms)
    waitFor("object.exists(':threeSliceButton_QPushButton')", 20000)
    test.compare(fix_bool(findObject(":threeSliceButton_QPushButton").enabled), ts)
    waitFor("object.exists(':splatterPlotButton_QPushButton')", 20000)
    test.compare(fix_bool(findObject(":splatterPlotButton_QPushButton").enabled), sp)

def check_time_controls(mode):
    waitFor("object.exists(':TimeControlWidget_Mantid::Vates::SimpleGui::TimeControlWidget')", 20000)
    test.compare(fix_bool(findObject(":TimeControlWidget_Mantid::Vates::SimpleGui::TimeControlWidget").enabled),
                 mode)

def check_slices(axisScaleName, indicatorsRequired):
    ext = None
    if axisScaleName[0] == "y":
        ext = ""
    if axisScaleName[0] == "x":
        ext = "_2"
    if axisScaleName[0] == "z":
        ext = "_3"

    graphScene = waitForObject(":_QGraphicsScene%s" % ext)
    indicatorsPresent = graphScene.items().size()
    test.compare(indicatorsPresent, indicatorsRequired)
