from sans.common.enums import (SANSInstrument)


class BeamCentreModel(object):
    def __init__(self):
        super(BeamCentreModel, self).__init__()
        self.reset_to_defaults_for_instrument()

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def reset_to_defaults_for_instrument(self, instrument = None):
        self._max_iterations = 10
        self._r_min = 60
        self._r_max = 280
        self._left_right = True
        self._up_down = True
        self._tolerance = 0.000125
        self._lab_pos_1 = ''
        self._lab_pos_2 = ''
        self._hab_pos_2 = ''
        self._hab_pos_1 = ''
        self.scale_1 = 1000
        self.scale_2 = 1000

        if instrument == SANSInstrument.LOQ:
            self.r_max = 200

        if instrument == SANSInstrument.LARMOR:
            self.scale_1 = 1.0

    def set_scaling(self, instrument):
        self.scale_1 = 1000
        self.scale_2 = 1000
        if instrument == SANSInstrument.LARMOR:
            self.scale_1 = 1.0

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
