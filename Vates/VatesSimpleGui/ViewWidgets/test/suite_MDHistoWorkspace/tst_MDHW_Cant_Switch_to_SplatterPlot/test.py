def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("MantidPlot")
    run_script("mdhistos_from_3D.py")

    get_workspace("TOPAZ_3680_3D", do_snooze=True)
    activate_vsi()
    check_mode_button_state("splatterPlot", False, "SplatterPlot disabled for MDHW")
    close_vsi()

    quit_program()