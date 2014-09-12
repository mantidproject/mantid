
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("TOPAZ_3680_5_sec_MDEW.nxs")

    switch_mode("threeSlice")
    check_mode_buttons(std=True, ms=True, ts=False, sp=True)

    mouseDrag(waitForObject(":mainRenderFrame.Viewport_pqQVTKWidget"), 219, 225, 0, 12, 1, Qt.LeftButton)
    mouseClick(waitForObject(":mainRenderFrame.Viewport_pqQVTKWidget"), 184, 191, 0, Qt.LeftButton)
    mouseDrag(waitForObject(":mainRenderFrame.Viewport_pqQVTKWidget"), 156, 214, 1, -20, 1, Qt.LeftButton)

    quit_program()

