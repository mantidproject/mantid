# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("MantidPlot")
    run_script("mdew_4D.py")

    workspace_name = "SEQ"
    get_workspace(workspace_name)
    activate_vsi()
    view_filter("Rebin")
    check_view_filter_button_state("Cut", False, "Cannot Cut without Clicking Apply")

    quit_program()