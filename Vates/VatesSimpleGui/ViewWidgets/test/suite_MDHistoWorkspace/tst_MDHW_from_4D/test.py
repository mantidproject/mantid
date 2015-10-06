def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("MantidPlot")
    run_script("mdhistos_from_4D.py")

    get_workspace("SEQ_4D_rebin")
    check_vsi_state(True, "VSI enabled for 4D rebinned MDHW")

    get_workspace("SEQ_3D_rebin")
    check_vsi_state(True, "VSI enabled for 3D rebinned MDHW")

    get_workspace("SEQ_2D_rebin")
    check_vsi_state(False, "VSI not enabled for 2D rebinned MDHW")

    get_workspace("SEQ_1D_rebin")
    check_vsi_state(False, "VSI not enabled for 1D rebinned MDHW")

    get_workspace("SEQ_3D_int")
    check_vsi_state(True, "VSI enabled for 3D integrated MDHW")

    get_workspace("SEQ_2D_int")
    check_vsi_state(False, "VSI not enabled for 2D integrated MDHW")

    get_workspace("SEQ_1D_int")
    check_vsi_state(False, "VSI not enabled for 1D integrated MDHW")

    quit_program()

