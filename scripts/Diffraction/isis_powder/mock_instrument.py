from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines import RunDetails


class MockInstrument(AbstractInst):

    def __init__(self, user_name, calibration_dir, output_dir):
        super(MockInstrument, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                             output_dir=output_dir)
        self.generate_cycle_dir_flag = False

    def _generate_output_file_name(self, run_number_string):
        return "test_output_string"

    def _apply_absorb_corrections(self, calibration_full_paths, ws_to_match):
        return None

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        return None

    def _get_run_details(self, run_number_string):
        return RunDetails.RunDetails(run_number=run_number_string)
