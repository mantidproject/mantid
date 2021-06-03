# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import Logger
from sans.common.enums import (FindDirectionEnum, DetectorType, SANSInstrument)
from sans.state.AllStates import AllStates
from sans.gui_logic.gui_common import (meter_2_millimeter, millimeter_2_meter, apply_selective_view_scaling,
                                       undo_selective_view_scaling)


class BeamCentreModel(object):
    logger = Logger("CentreFinder")

    def __init__(self, SANSCentreFinder):
        super(BeamCentreModel, self).__init__()
        self._max_iterations = 10
        self._r_min = 0
        self._r_max = 0
        self._left_right = True
        self._up_down = True
        self._tolerance = 1.251E-07 # metres
        self._lab_pos_1 = ''
        self._lab_pos_2 = ''
        self._hab_pos_2 = ''
        self._hab_pos_1 = ''
        self.COM = False
        self.verbose = False
        self.q_min = 0.01
        self.q_max = 0.1
        self._component = DetectorType.LAB
        self.update_lab = True
        self.update_hab = True
        self.instrument = None

        self.reset_inst_defaults(instrument=SANSInstrument.NO_INSTRUMENT)

        self.SANSCentreFinder = SANSCentreFinder

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def reset_inst_defaults(self, instrument):
        if instrument is SANSInstrument.LOQ:
            self._r_min = 0.096 # metres
            self._r_max = 0.216 # metres

            # TODO HAB on LOQ prefers 96-750
        else:
            # All other instruments hard-code this as follows
            self._r_min = 0.06 # metres
            self._r_max = 0.280 # metres

        self.instrument = instrument

    def find_beam_centre(self, state: AllStates):
        """
        This is called from the GUI and runs the find beam centre algorithm given a state model and a beam_centre_model object.

        :param state: A SANS state object
        :param beam_centre_model: An instance of the BeamCentreModel class.
        :returns: The centre position found.
        """
        centre_finder = self.SANSCentreFinder()
        find_direction = self.get_finder_direction()
        if not find_direction:
            self.logger.error("Have chosen no find direction exiting early")
            return

        pos_1 = self._lab_pos_1 if self.component is DetectorType.LAB else self._hab_pos_1
        pos_2 = self._lab_pos_2 if self.component is DetectorType.LAB else self._hab_pos_2

        if self.COM:
            centre = centre_finder(state, r_min=self.r_min, r_max=self.r_max,
                                   max_iter=self.max_iterations,
                                   x_start=pos_1, y_start=pos_2,
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
                                   max_iter=self.max_iterations, x_start=pos_1,
                                   y_start=pos_2, tolerance=self.tolerance,
                                   find_direction=find_direction, reduction_method=True,
                                   verbose=self.verbose, component=self.component)

        self._update_centre_positions(results=centre)

    def _update_centre_positions(self, results):
        if self.component is DetectorType.LAB:
            self._lab_pos_1 = results["pos1"]
            self._lab_pos_2 = results["pos2"]
        elif self.component is DetectorType.HAB:
            self._hab_pos_1 = results['pos1']
            self._hab_pos_2 = results['pos2']
        else:
            raise RuntimeError("Unexpected detector type, got %r" % results)

    def get_finder_direction(self):
        find_direction = None
        if self.up_down and self.left_right:
            find_direction = FindDirectionEnum.ALL
        elif self.up_down:
            find_direction = FindDirectionEnum.UP_DOWN
        elif self.left_right:
            find_direction = FindDirectionEnum.LEFT_RIGHT

        return find_direction

    @property
    def max_iterations(self):
        return self._max_iterations

    @max_iterations.setter
    def max_iterations(self, value):
        self._max_iterations = value

    @property
    def r_min(self):
        return meter_2_millimeter(self._r_min)

    @r_min.setter
    def r_min(self, value):
        self._r_min = millimeter_2_meter(value)

    @property
    def r_max(self):
        return meter_2_millimeter(self._r_max)

    @r_max.setter
    def r_max(self, value):
        self._r_max = millimeter_2_meter(value)

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
        return meter_2_millimeter(self._tolerance)

    @tolerance.setter
    def tolerance(self, value):
        self._tolerance = millimeter_2_meter(value)

    @property
    @apply_selective_view_scaling
    def lab_pos_1(self):
        return self._lab_pos_1 if self._lab_pos_1 else ''

    @lab_pos_1.setter
    @undo_selective_view_scaling
    def lab_pos_1(self, value):
        self._lab_pos_1 = value

    @property
    @apply_selective_view_scaling
    def lab_pos_2(self):
        return self._lab_pos_2 if self._lab_pos_2 else ''

    @lab_pos_2.setter
    @undo_selective_view_scaling
    def lab_pos_2(self, value):
        self._lab_pos_2 = value

    @property
    @apply_selective_view_scaling
    def hab_pos_1(self):
        return self._hab_pos_1 if self._hab_pos_1 else ''

    @hab_pos_1.setter
    @undo_selective_view_scaling
    def hab_pos_1(self, value):
        self._hab_pos_1 = value

    @property
    @apply_selective_view_scaling
    def hab_pos_2(self):
        return self._hab_pos_2 if self._hab_pos_2 else ''

    @hab_pos_2.setter
    @undo_selective_view_scaling
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
