
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("TOPAZ_3680_5_sec_MDEW.nxs")
    switch_mode("splatterPlot")
    check_mode_buttons(std=True, ms=True, ts=True, sp=False)

    set_ptw_lineedit_property(10000, "Number of Points")
    apply_ptw_settings()

    mouseDrag(waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 505, 223, -35, 9, 1, Qt.LeftButton)
    quit_program()

