
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("TOPAZ_3680_5_sec_MDEW.nxs")
    switch_mode("splatterPlot")
    check_mode_buttons(std=True, ms=True, ts=True, sp=False)

    set_ptw_lineedit_property(10000, "Number of Points")
    apply_ptw_settings()

    open_file("TOPAZ_3680_10_sec_40.peaks")

    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    mouseDrag(waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 384, 329, -79, 4, 1, Qt.LeftButton)

    quit_program()
