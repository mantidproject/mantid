def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("MantidPlot")
    run_script("viewable_mdhistos.py")

    get_workspace("SEQ_4D_rebin")
    activate_vsi()
    check_view_filter_button_state("Rebin", False, "Rebin button disabled for 4D MDHW")
    close_vsi()

    get_workspace("SEQ_3D_rebin")
    activate_vsi()
    check_view_filter_button_state("Rebin", False, "Rebin button disabled for 3D MDHW")
    close_vsi()

    get_workspace("SEQ_3D_int")
    activate_vsi()
    check_view_filter_button_state("Rebin", False, "Rebin button disabled for 3D MDHW")
    close_vsi()

    quit_program()
