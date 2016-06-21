def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("MantidPlot")
    run_script("mdew_3D.py")

    workspace_name = "TOPAZ_3680"
    get_workspace(workspace_name, do_snooze=True)
    activate_vsi()
    view_filter("Rebin")
    apply_ptw_settings()
    switch_mode("splatterPlot")

    # Try to overplot
    get_workspace(workspace_name)
    activate_vsi()

    # Check for warning dialog box
    test.compare(fix_bool(waitFor("object.exists(':_QMessageBox')", 20000)), True,
                 "Warning dialog is shown")
    clickButton(waitForObject(":splitter_2.OK_QPushButton"))

    # Check VSI states
    check_mode_button_state("splatterPlot", False, "SplatterPlot mode should still be disabled")
    # MAR 2012/04/13 This doesn't seem to work any more.
    #pipeline_filter = get_pipeline_filter_at_position(3)
    #check_filter_selection(pipeline_filter, True, "SplatterPlot filter should be selected.")

    close_vsi()

    quit_program()
