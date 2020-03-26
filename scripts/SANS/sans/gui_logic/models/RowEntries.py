# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from six import iteritems, iterkeys

from sans.common.enums import RowState, SampleShape
from sans.common.file_information import SANSFileInformationFactory
from sans.gui_logic.models.RowOptionsModel import RowOptionsModel
from mantid.kernel import Logger


class _UserEntries(object):
    """
    POD type for the row entries found on the main GUI
    """
    def __init__(self):
        self.can_direct = None
        self.can_scatter = None
        self.can_transmission = None

        self.sample_direct = None
        self.sample_scatter = None
        self.sample_transmission = None

        self.output_name = None
        self.user_file = None

        self.sample_height = None
        self._sample_shape = None
        self.sample_thickness = None
        self.sample_width = None

        self.can_direct_period = None
        self.can_scatter_period = None
        self.can_transmission_period = None

        self.sample_direct_period = None
        self.sample_scatter_period = None
        self.sample_transmission_period = None


class RowEntries(_UserEntries):
    _data_vars = vars(_UserEntries())
    _start_observing = False
    _logger = Logger("Row Entry")

    def __init__(self, **kwargs):
        super(RowEntries, self).__init__()
        self._options = RowOptionsModel()

        self.tool_tip = None
        self.state = RowState.UNPROCESSED

        self._start_observing = True  # Allow init to use setattr without validation

        for k, v in iteritems(kwargs):
            setattr(self, k, v)

    @property
    def file_information(self):
        # TODO this should be removed from row entries - it's an internal state not a GUI one
        file_factory = SANSFileInformationFactory()
        return file_factory.create_sans_file_information(self.sample_scatter)

    @property
    def options(self):
        return self._options

    @options.setter
    def options(self, value):
        assert isinstance(value, RowOptionsModel), \
            "Expected a RowOptionsModel, got %r" %value
        self._options = value

    @property
    def sample_shape(self):
        return self._sample_shape

    @sample_shape.setter
    def sample_shape(self, val):
        if not val:
            self._sample_shape = None
            return

        if isinstance(val, SampleShape):
            self._sample_shape = val
            return

        try:
            self._sample_shape = SampleShape(val)
        except ValueError as e:
            self._logger.error(str(e))
            self._sample_shape = None

    def is_multi_period(self):
        return any((self.sample_scatter_period, self.sample_transmission_period, self.sample_direct_period,
                    self.can_scatter_period, self.can_transmission_period, self.can_direct_period))

    def is_empty(self):
        return not any(getattr(self, attr) for attr in iterkeys(self._data_vars))

    def reset_row_state(self):
        self.state = RowState.UNPROCESSED
        self.tool_tip = None

    def __setattr__(self, key, value):
        if self._start_observing and key in self._data_vars:
            self.reset_row_state()
        if self._start_observing and not hasattr(self, key):
            raise AttributeError("{0}".format(key))

        return super(RowEntries, self).__setattr__(key, value)
