
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    openFile("MDEW_4D.nxs")

    clickButton(waitForObject(":splitter_2.Rebin_QPushButton"))
    set_ptw_lineedit_property(":ScrollArea.Bins_QLineEdit_4", "3")
    apply_ptw_settings()

    switch_mode("multiSlice")
    check_mode_buttons(std=True, ms=False, ts=True, sp=True)
    
    make_slice("xAxisWidget", 0.0)
    make_slice("yAxisWidget", 0.0)
    make_slice("zAxisWidget", 0.0)
    apply_ptw_settings()
    
    mouseDrag(waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 137, 170, -95, 9, 1, Qt.LeftButton)
    
    quitProgram()

