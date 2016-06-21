def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("MantidPlot")
    run_script("viewable_mdhistos.py")

    workspace_name = "SEQ_4D_rebin"
    get_workspace(workspace_name)
    activate_vsi()
    switch_mode("multiSlice")
    make_slice("xAxisWidget", 0.05)
    slice_number = 1
    activate_sv(slice_number)
    check_sv_opened(workspace_name, slice_number, True, "SliceView launched from MultiSlice mode for MDHW.")

    quit_program()