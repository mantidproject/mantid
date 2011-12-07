
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    openFile("TOPAZ_3680_5_sec_MDEW.nxs")
    switch_mode("splatterPlot")
    check_mode_buttons(std=True, ms=True, ts=True, sp=False)
    
    mouseClick(waitForObject(":ScrollArea.Number of Points_QLineEdit"), 90, 20, 0, Qt.LeftButton)
    type(waitForObject(":ScrollArea.Number of Points_QLineEdit"), "0")
    clickButton(waitForObject(":objectInspector.Apply_QPushButton"))
    
    mouseDrag(waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 505, 223, -35, 9, 1, Qt.LeftButton)
    quitProgram()

