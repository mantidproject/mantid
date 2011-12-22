
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("MDEW_4D.nxs")
    clickButton(waitForObject(":splitter_2.Rebin_QPushButton"))
    set_ptw_lineedit_property(100, "Bins")
    set_ptw_lineedit_property(100, "Bins", "2")
    set_ptw_lineedit_property(100, "Bins", "3")
    set_ptw_lineedit_property(3, "Bins", "4")
    apply_ptw_settings()
    
    switch_mode("threeSlice")
    check_mode_buttons(std=True, ms=True, ts=False, sp=True)
    
    mouseDrag(waitForObject(":mainRenderFrame.Viewport_pqQVTKWidget"), 219, 225, 0, 12, 1, Qt.LeftButton)
    mouseClick(waitForObject(":mainRenderFrame.Viewport_pqQVTKWidget"), 184, 191, 0, Qt.LeftButton)
    mouseDrag(waitForObject(":mainRenderFrame.Viewport_pqQVTKWidget"), 156, 214, 1, -20, 1, Qt.LeftButton)
    
    quit_program()

