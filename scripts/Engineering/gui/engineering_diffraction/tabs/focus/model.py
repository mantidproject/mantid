# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from mantid.simpleapi import EnggFocus, Load, logger

SAMPLE_RUN_WORKSPACE_NAME = "engggui_focusing_input_ws"
FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_bank_"


class FocusModel(object):
    def focus_run(self, sample_path, banks, plot_output, instrument, rb_number, current_calib):
        vanadium_path = current_calib["vanadium_path"]
        if vanadium_path is None:
            return
        integration_workspace, curves_workspace = vanadium_corrections.fetch_correction_workspaces(
            vanadium_path, instrument)
        sample_workspace = self._load_focus_sample_run(sample_path)
        for name in banks:
            output_workspace_name = FOCUSED_OUTPUT_WORKSPACE_NAME + str(name)
            self._run_focus(sample_workspace, output_workspace_name, integration_workspace, curves_workspace, name)

    @staticmethod
    def _run_focus(input_workspace, output_workspace, vanadium_integration_ws,
                   vanadium_curves_ws, bank):
        try:
            return EnggFocus(InputWorkspace=input_workspace,
                             OutputWorkspace=output_workspace,
                             VanIntegrationWorkspace=vanadium_integration_ws,
                             VanCurvesWorkspace=vanadium_curves_ws,
                             Bank=bank)
        except RuntimeError as e:
            logger.error(
                "Error in focusing, Could not run the EnggFocus algorithm successfully for bank " +
                str(bank) + ". Error Description: " + str(e))
            raise RuntimeError()

    @staticmethod
    def _load_focus_sample_run(sample_path):
        try:
            return Load(Filename=sample_path, OutputWorkspace=SAMPLE_RUN_WORKSPACE_NAME)
        except RuntimeError as e:
            logger.error(
                "Error while loading sample data for focusing. Could not load the sample with filename: "
                + sample_path + ". Error Description: " + str(e))
            raise RuntimeError
