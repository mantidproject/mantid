from sans.common.enums import (SANSInstrument, FindDirectionEnum, DetectorType)
from mantid.kernel import (Logger)
from sans.common.file_information import get_instrument_paths_for_sans_file
from sans.common.xml_parsing import get_named_elements_from_ipf_file


class BeamCentreModel(object):
    def __init__(self, SANSCentreFinder):
        super(BeamCentreModel, self).__init__()
        self.reset_to_defaults_for_instrument()
        self.SANSCentreFinder = SANSCentreFinder

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def reset_to_defaults_for_instrument(self, state_data = None):
        r_range = {}
        instrument = None

        if state_data:
            instrument_file_path = get_instrument_paths_for_sans_file(state_data.sample_scatter)
            r_range = get_named_elements_from_ipf_file(instrument_file_path[1],
                                                       ["beam_centre_radius_min", "beam_centre_radius_max"], float)
            instrument = state_data.instrument

        self._max_iterations = 10
        self._r_min = r_range["beam_centre_radius_min"] if "beam_centre_radius_min" in r_range else 60
        self._r_max = r_range["beam_centre_radius_max"] if "beam_centre_radius_max" in r_range else 280
        self._left_right = True
        self._up_down = True
        self._tolerance = 0.0001251
        self._lab_pos_1 = ''
        self._lab_pos_2 = ''
        self._hab_pos_2 = ''
        self._hab_pos_1 = ''
        self.scale_1 = 1000
        self.scale_2 = 1000
        self.COM = False
        self.verbose = False
        self.q_min = 0.01
        self.q_max = 0.1
        self._component = DetectorType.LAB
        self.update_lab = True
        self.update_hab = True

        if instrument == SANSInstrument.LARMOR:
            self.scale_1 = 1.0

    def set_scaling(self, instrument):
        self.scale_1 = 1000
        self.scale_2 = 1000
        if instrument == SANSInstrument.LARMOR:
            self.scale_1 = 1.0

    def find_beam_centre(self, state):
        """
        This is called from the GUI and runs the find beam centre algorithm given a state model and a beam_centre_model object.

        :param state: A SANS state object
        :param beam_centre_model: An instance of the BeamCentreModel class.
        :returns: The centre position found.
        """
        centre_finder = self.SANSCentreFinder()
        find_direction = None
        if self.up_down and self.left_right:
            find_direction = FindDirectionEnum.All
        elif self.up_down:
            find_direction = FindDirectionEnum.Up_Down
        elif self.left_right:
            find_direction = FindDirectionEnum.Left_Right
        else:
            logger = Logger("CentreFinder")
            logger.notice("Have chosen no find direction exiting early")
            return {"pos1": self.lab_pos_1, "pos2": self.lab_pos_2}

        if self.COM:
            centre = centre_finder(state, r_min=self.r_min, r_max=self.r_max,
                                   max_iter=self.max_iterations,
                                   x_start=self.lab_pos_1, y_start=self.lab_pos_2,
                                   tolerance=self.tolerance,
                                   find_direction=find_direction, reduction_method=False, component=self.component)

            centre = centre_finder(state, r_min=self.r_min, r_max=self.r_max,
                                   max_iter=self.max_iterations,
                                   x_start=centre['pos1'], y_start=centre['pos2'],
                                   tolerance=self.tolerance,
                                   find_direction=find_direction, reduction_method=True,
                                   verbose=self.verbose, component=self.component)
        else:
            centre = centre_finder(state, r_min=self.r_min, r_max=self.r_max,
                                   max_iter=self.max_iterations, x_start=self.lab_pos_1,
                                   y_start=self.lab_pos_2, tolerance=self.tolerance,
                                   find_direction=find_direction, reduction_method=True,
                                   verbose=self.verbose, component=self.component)
        return centre

    @property
    def max_iterations(self):
        return self._max_iterations

    @max_iterations.setter
    def max_iterations(self, value):
        self._max_iterations = value

    @property
    def r_min(self):
        return self._r_min

    @r_min.setter
    def r_min(self, value):
        self._r_min = value

    @property
    def r_max(self):
        return self._r_max

    @r_max.setter
    def r_max(self, value):
        self._r_max = value

    @property
    def q_min(self):
        return self._q_min

    @q_min.setter
    def q_min(self, value):
        self._q_min = value

    @property
    def q_max(self):
        return self._q_max

    @q_max.setter
    def q_max(self, value):
        self._q_max = value

    @property
    def left_right(self):
        return self._left_right

    @left_right.setter
    def left_right(self, value):
        self._left_right = value

    @property
    def up_down(self):
        return self._up_down

    @up_down.setter
    def up_down(self, value):
        self._up_down = value

    @property
    def verbose(self):
        return self._verbose

    @verbose.setter
    def verbose(self, value):
        self._verbose = value

    @property
    def COM(self):
        return self._COM

    @COM.setter
    def COM(self, value):
        self._COM = value

    @property
    def tolerance(self):
        return self._tolerance

    @tolerance.setter
    def tolerance(self, value):
        self._tolerance = value

    @property
    def lab_pos_1(self):
        return self._lab_pos_1

    @lab_pos_1.setter
    def lab_pos_1(self, value):
        self._lab_pos_1 = value

    @property
    def lab_pos_2(self):
        return self._lab_pos_2

    @lab_pos_2.setter
    def lab_pos_2(self, value):
        self._lab_pos_2 = value

    @property
    def hab_pos_1(self):
        return self._hab_pos_1

    @hab_pos_1.setter
    def hab_pos_1(self, value):
        self._hab_pos_1 = value

    @property
    def hab_pos_2(self):
        return self._hab_pos_2

    @hab_pos_2.setter
    def hab_pos_2(self, value):
        self._hab_pos_2 = value

    @property
    def component(self):
        return self._component

    @component.setter
    def component(self, value):
        self._component = value

    @property
    def update_hab(self):
        return self._update_hab

    @update_hab.setter
    def update_hab(self, value):
        self._update_hab = value

    @property
    def update_lab(self):
        return self._update_lab

    @update_lab.setter
    def update_lab(self, value):
        self._update_lab = value
