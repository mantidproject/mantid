# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans.common.file_information import SANSFileInformation
from mantid.kernel import DateAndTime
from sans.common.enums import SANSFacility, SANSInstrument, FileType, SampleShape


class SANSFileInformationMock(SANSFileInformation):
    def __init__(
        self,
        instrument=SANSInstrument.LOQ,
        facility=SANSFacility.ISIS,
        run_number=00000,
        file_name="file_name",
        height=8.0,
        width=8.0,
        thickness=1.0,
        shape=SampleShape.FLAT_PLATE,
        date="2012-10-22T22:41:27",
        periods=1,
        event_mode=True,
        added_data=False,
    ):
        super(SANSFileInformationMock, self).__init__(file_name)
        self._instrument = instrument
        self._facility = facility
        self._height = height
        self._width = width
        self._thickness = thickness
        self._shape = shape
        self._date = date
        self._file_name = file_name
        self._periods = periods
        self._event_mode = event_mode
        self._added_data = added_data

    def get_file_name(self):
        return self._file_name

    def get_instrument(self):
        return self._instrument

    def get_facility(self):
        return self._facility

    def get_date(self):
        return DateAndTime(self._date)

    def get_number_of_periods(self):
        return self._periods

    def get_type(self):
        return FileType.ISIS_NEXUS

    def is_event_mode(self):
        return self._event_mode

    def is_added_data(self):
        return self._added_data

    def get_height(self):
        return self._height

    def get_width(self):
        return self._width

    def get_thickness(self):
        return self._thickness

    def get_shape(self):
        return self._shape

    def _get_run_number_from_file(self, file_name):
        return "12345"
