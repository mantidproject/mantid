# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import Logger
from sans_core.common.enums import FindDirectionEnum, DetectorType, SANSInstrument
from mantidqtinterfaces.sans_isis.gui_logic.gui_common import (
    meter_2_millimeter,
    millimeter_2_meter,
    apply_selective_view_scaling,
    undo_selective_view_scaling,
)
from mantidqtinterfaces.sans_isis.gui_logic.models.async_workers.beam_centre_async import BeamCentreFields


class BeamCentreModel(object):
    logger = Logger("CentreFinder")

    def __init__(self):
        super(BeamCentreModel, self).__init__()
        self._max_iterations = 10
        self._r_min = 0
        self._r_max = 0
        self._left_right = True
        self._up_down = True
        self._tolerance = 1.251e-07  # metres
        self._rear_pos_1 = ""
        self._rear_pos_2 = ""
        self._front_pos_2 = ""
        self._front_pos_1 = ""
        self.COM = False
        self.verbose = False
        self.q_min = 0.01
        self.q_max = 0.1
        # Where LAB == Rear and HAB == Front
        self._component = DetectorType.LAB
        self.update_rear = True
        self.update_front = True
        self.instrument = None

        self.reset_inst_defaults(instrument=SANSInstrument.NO_INSTRUMENT)

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def reset_inst_defaults(self, instrument):
        if instrument is SANSInstrument.LOQ:
            self._r_min = 0.096  # metres
            self._r_max = 0.216  # metres

            # TODO front on LOQ prefers 96-750
        else:
            # All other instruments hard-code this as follows
            self._r_min = 0.06  # metres
            self._r_max = 0.280  # metres

        self.instrument = instrument

    def update_centre_positions(self, results):
        if self.component is DetectorType.LAB:
            self._rear_pos_1 = results["pos1"]
            self._rear_pos_2 = results["pos2"]
        elif self.component is DetectorType.HAB:
            self._front_pos_1 = results["pos1"]
            self._front_pos_2 = results["pos2"]
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

    def pack_beam_centre_settings(self) -> BeamCentreFields:
        # We pack into a separate object so that:
        # 1. we don't share memory across threads (so free thread-safety)
        # 2. We only move the attrs that are relevant across threads
        return BeamCentreFields(
            component=self.component,
            centre_of_mass=self.COM,
            find_direction=self.get_finder_direction(),
            lab_pos_1=self._rear_pos_1,
            lab_pos_2=self._rear_pos_2,
            hab_pos_1=self._front_pos_1,
            hab_pos_2=self._front_pos_2,
            max_iterations=self.max_iterations,
            r_min=self.r_min,
            r_max=self.r_max,
            tolerance=self.tolerance,
            verbose=self.verbose,
        )

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
    def rear_pos_1(self):
        return self._rear_pos_1 if self._rear_pos_1 is not None else ""

    @rear_pos_1.setter
    @undo_selective_view_scaling
    def rear_pos_1(self, value):
        self._rear_pos_1 = value

    @property
    @apply_selective_view_scaling
    def rear_pos_2(self):
        return self._rear_pos_2 if self._rear_pos_2 is not None else ""

    @rear_pos_2.setter
    @undo_selective_view_scaling
    def rear_pos_2(self, value):
        self._rear_pos_2 = value

    @property
    @apply_selective_view_scaling
    def front_pos_1(self):
        return self._front_pos_1 if self._front_pos_1 is not None else ""

    @front_pos_1.setter
    @undo_selective_view_scaling
    def front_pos_1(self, value):
        self._front_pos_1 = value

    @property
    @apply_selective_view_scaling
    def front_pos_2(self):
        return self._front_pos_2 if self._front_pos_2 is not None else ""

    @front_pos_2.setter
    @undo_selective_view_scaling
    def front_pos_2(self, value):
        self._front_pos_2 = value

    @property
    def component(self):
        return self._component

    @component.setter
    def component(self, value):
        self._component = value
