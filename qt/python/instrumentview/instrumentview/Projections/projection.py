import numpy as np


class projection:
    _component_info = None
    _detector_indices = None
    _sample_position = None
    _projection_axis = None
    _x_axis = None
    _y_axis = None
    _u_period = np.pi
    _detector_x_coordinates = None
    _detector_y_coordinates = None
    _x_range = None
    _y_range = None

    def __init__(self, workspace, detector_indices):
        self._component_info = workspace.componentInfo()
        self._sample_position = np.array(self._component_info.samplePosition())
        self._detector_indices = detector_indices

    def _calculate_axes(self):
        position = np.array(self._component_info.position(0))
        z = position.dot(self._projection_axis)
        if z == 0 or np.abs(z) == np.linalg.norm(position):
            # Find the shortest projection of the projection axis and direct the x axis along it
            if np.abs(self._projection_axis[2]) < np.abs(self._projection_axis[1]):
                self._x_axis = np.array([0, 0, 1])
            elif np.abs(self._projection_axis[1]) < np.abs(self._projection_axis[0]):
                self._x_axis = np.array([0, 1, 0])
            else:
                self._x_axis = np.array([1, 0, 0])
        else:
            x_axis = position - z * self._projection_axis
            self._x_axis = x_axis / np.linalg.norm(x_axis)

        self._y_axis = np.cross(self._projection_axis, self._x_axis)

    def _calculate_detector_coordinates(self):
        self._detector_x_coordinates = np.array([])
        self._detector_y_coordinates = np.array([])

    def _find_and_correct_x_gap(self):
        if self._u_period == 0:
            return

        number_of_bins = 1000
        x_bins = [False for i in range(number_of_bins)]
        bin_width = np.abs(self._x_range[1] - self._x_range[0]) / (number_of_bins - 1)
        if bin_width == 0.0:
            return

        for i in range(len(self._detector_indices)):
            if not self._component_info.hasValidShape(self._detector_indices[i]):
                continue
            x = self._detector_x_coordinates[i]
            bin_i = int((x - self._x_range[0]) / bin_width)
            x_bins[bin_i] = True

        i_from = 0
        i_to = 0
        i0 = 0
        in_gap = False

        for i in range(number_of_bins):
            if not x_bins[i]:
                if not in_gap:
                    i0 = i
                in_gap = True
            else:
                if in_gap and i_to - i_from < i - i0:
                    i_from = i0  # First bin in the gap
                    i_to = i  # First bin after the gap
                in_gap = False

        x_from = self._x_range[0] + i_from * bin_width
        x_to = self._x_range[0] + i_to * bin_width
        if x_to - x_from > self._u_period - (self._x_range[1] - self._x_range[0]):
            self._x_range = (x_to, x_from)
            if self._x_range[0] > self._x_range[1]:
                self._x_range = (self._x_range[0], self._x_range[1] + self._u_period)
            for i in range(len(self._detector_indices)):
                if not self._component_info.hasValidShape(self._detector_indices[i]):
                    continue
                self._apply_x_correction(i)

    def _apply_x_correction(self, i: int) -> None:
        x = self._detector_x_coordinates[i]
        if self._u_period == 0:
            return
        if x < self._x_range[0]:
            periods = np.floor((self._x_range[1] - x) / self._u_period) * self._u_period
            x = x + periods
        if x > self._x_range[1]:
            periods = np.floor((x - self._x_range[0]) / self._u_period) * self._u_period
            x = x - periods
        self._detector_x_coordinates[i] = x

    def coordinate_for_detector(self, detector_index: int) -> tuple[float, float]:
        return (self._detector_x_coordinates[detector_index], self._detector_y_coordinates[detector_index])
