from isis_powder.abstract_inst import AbstractInst


class MockInstrument(AbstractInst):

    def __init__(self, user_name, calibration_dir, output_dir, default_ext=""):
        super(MockInstrument, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                             output_dir=output_dir, default_input_ext=default_ext)
        self.generate_cycle_dir_flag = False

    def _get_lambda_range(self):
        return None

    def _get_focus_tof_binning(self):
        return None

    def get_create_van_tof_binning(self):
        return None

    def get_default_group_names(self):
        return None

    def get_run_details(self, run_number_string):
        # This is here to help remind people of the dict that is expected
        calibration_details = {"calibration": "cal",
                               "grouping": "group",
                               "vanadium_absorption": "van_absorb",
                               "vanadium": "van"}
        return calibration_details

    @staticmethod
    def generate_input_file_name(run_number):
        return "generate_inst_file_name" + str(run_number)

    @staticmethod
    def get_num_of_banks(instrument=''):
        return None

    @staticmethod
    def _get_label_information(run_number):
        # This is here to help remind people of the dict format
        cycle_information = {"cycle" : "123",
                             "instrument_version": "test_v1"}
        return cycle_information

    def test_set_raw_data_dir(self, new_dir):
        # Used for testing to set a new raw_data_dir
        self._raw_data_dir = new_dir
