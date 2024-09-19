# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass
from SANS.sans.common.enums import RowState, SampleShape
from SANS.sans.common.file_information import SANSFileInformationFactory
from SANS.sans.common.RowOptionsModel import RowOptionsModel
from mantid.kernel import Logger


@dataclass(eq=False)
class _UserEntries(object):
    """
    POD type for the row entries found on the main GUI
    """

    def __init__(self):
        self.can_direct: int = None
        self.can_scatter: int = None
        self.can_transmission: int = None

        self.sample_direct: int = None
        self.sample_scatter: int = None
        self.sample_transmission: int = None

        self.output_name: str = None
        self.user_file: str = None

        self.sample_height: float = None
        self._sample_shape: SampleShape = None
        self.sample_thickness: float = None
        self.sample_width: float = None

        self.background_ws: str = None
        self.scale_factor: float = None

        self.can_direct_period: str = None
        self.can_scatter_period: str = None
        self.can_transmission_period: str = None

        self.sample_direct_period: str = None
        self.sample_scatter_period: str = None
        self.sample_transmission_period: str = None

    def __eq__(self, other):
        # Running with Python 3.6 does not automatically generate the __annotations__
        # member, causing dataclass to skip our fields. Instead we can manually do this
        # for the moment
        if isinstance(other, _UserEntries):
            return (
                self.can_direct,
                self.can_scatter,
                self.can_transmission,
                self.sample_direct,
                self.sample_scatter,
                self.sample_transmission,
                self.output_name,
                self.user_file,
                self.sample_height,
                self._sample_shape,
                self.sample_thickness,
                self.sample_width,
                self.background_ws,
                self.scale_factor,
                self.can_direct_period,
                self.can_scatter_period,
                self.can_transmission_period,
                self.sample_direct,
                self.sample_scatter_period,
                self.sample_transmission_period,
            ) == (
                other.can_direct,
                other.can_scatter,
                other.can_transmission,
                other.sample_direct,
                other.sample_scatter,
                other.sample_transmission,
                other.output_name,
                other.user_file,
                other.sample_height,
                other._sample_shape,
                other.sample_thickness,
                other.sample_width,
                other.background_ws,
                other.scale_factor,
                other.can_direct_period,
                other.can_scatter_period,
                other.can_transmission_period,
                other.sample_direct,
                other.sample_scatter_period,
                other.sample_transmission_period,
            )

    def __hash__(self):
        # We want to store "duplicate" hashable types
        return id(self)


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

        for k, v in kwargs.items():
            setattr(self, k, v)

    def __eq__(self, other):
        if isinstance(other, RowEntries):
            return (self.tool_tip, self.state) == (other.tool_tip, other.state) and super().__eq__(other)

    def __hash__(self):
        return id(self)

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
        assert isinstance(value, RowOptionsModel), "Expected a RowOptionsModel, got %r" % value
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
        return any(
            (
                self.sample_scatter_period,
                self.sample_transmission_period,
                self.sample_direct_period,
                self.can_scatter_period,
                self.can_transmission_period,
                self.can_direct_period,
            )
        )

    def is_empty(self):
        return not any(getattr(self, attr) for attr in self._data_vars.keys())

    def reset_row_state(self):
        self.state = RowState.UNPROCESSED
        self.tool_tip = None

    def __setattr__(self, key, value):
        if self._start_observing and key in self._data_vars:
            self.reset_row_state()
        if self._start_observing and not hasattr(self, key):
            raise AttributeError("{0}".format(key))

        return super(RowEntries, self).__setattr__(key, value)
