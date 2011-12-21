
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("MDEW_4D.nxs")

    clickButton(waitForObject(":splitter_2.Rebin_QPushButton"))
    set_ptw_lineedit_property(":ScrollArea.Bins_QLineEdit", 100)
    set_ptw_lineedit_property(":ScrollArea.Bins_QLineEdit_2", 100)
    set_ptw_lineedit_property(":ScrollArea.Bins_QLineEdit_3", 100)
    set_ptw_lineedit_property(":ScrollArea.Bins_QLineEdit_4", 3)
    apply_ptw_settings()

    switch_mode("multiSlice")
    check_mode_buttons(std=True, ms=False, ts=True, sp=True)
    
    make_slice("xAxisWidget", 0.0)
    make_slice("yAxisWidget", 0.0)
    make_slice("zAxisWidget", 0.0)
    make_slice("xAxisWidget", 1.0)
    make_slice("yAxisWidget", 1.0)
    make_slice("xAxisWidget", -1.0)
    apply_ptw_settings()
    
    check_slices("xAxisWidget", 3)
    check_slices("yAxisWidget", 2)
    check_slices("zAxisWidget", 1)
    
    mouseDrag(waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 137, 170, -95, 9, 1, Qt.LeftButton)
    
    quit_program()

