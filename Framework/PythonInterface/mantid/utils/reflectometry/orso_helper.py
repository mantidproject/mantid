# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from datetime import datetime, timezone
from typing import Optional, Union, List
import re
from orsopy.fileio.data_source import DataSource, Person, Experiment, Sample, Measurement
from orsopy.fileio import Reduction, Software
from orsopy.fileio.orso import Orso, OrsoDataset, save_orso, save_nexus
from orsopy.fileio.base import Column, ErrorColumn, File

from mantid.kernel import version
from enum import Enum


class MantidORSOSaver:
    ASCII_FILE_EXT = ".ort"
    NEXUS_FILE_EXT = ".orb"

    def __init__(self, filename: str, comment: str = None) -> None:
        self._filename = filename
        self._comment = comment
        self._datasets = []

    @property
    def filename(self) -> str:
        return self._filename

    def add_dataset(self, dataset: "MantidORSODataset") -> None:
        self._datasets.append(dataset.dataset)

    def save_orso_ascii(self) -> None:
        save_orso(datasets=self._datasets, fname=self._get_filename_with_extension(self.ASCII_FILE_EXT), comment=self._comment)

    def save_orso_nexus(self) -> None:
        save_nexus(datasets=self._datasets, fname=self._get_filename_with_extension(self.NEXUS_FILE_EXT), comment=self._comment)

    def _get_filename_with_extension(self, extension):
        return self._filename if self._filename.endswith(extension) else f"{self._filename}{extension}"

    @staticmethod
    def is_supported_extension(filename):
        return filename.endswith(MantidORSOSaver.ASCII_FILE_EXT) or filename.endswith(MantidORSOSaver.NEXUS_FILE_EXT)


class MantidORSODataset:
    PROBE_NEUTRON = "neutron"
    SOFTWARE_NAME = "Mantid"

    _RUN_START_LOG = "run_start"
    _DATETIME_FORMAT = "%Y-%m-%dT%H:%M:%S"

    def __init__(
        self,
        dataset_name: str,
        data_columns: "MantidORSODataColumns",
        ws,
        reduction_timestamp: datetime,
        creator_name: str,
        creator_affiliation: str,
    ) -> None:
        self._data_columns = data_columns
        self._header = None

        self._create_mandatory_header(ws, dataset_name, reduction_timestamp, creator_name, creator_affiliation)

    @property
    def dataset(self) -> OrsoDataset:
        return OrsoDataset(info=self._header, data=self._data_columns.data)

    def set_facility(self, facility: str) -> None:
        self._header.data_source.experiment.facility = facility

    def set_proposal_id(self, proposal_id: str) -> None:
        self._header.data_source.experiment.proposalID = proposal_id

    def set_doi(self, doi: str) -> None:
        self._header.data_source.experiment.doi = doi

    def set_reduction_call(self, call: str) -> None:
        self._header.reduction.call = call

    def add_measurement_data_file(self, filename: str, timestamp: Optional[datetime] = None, comment: Optional[str] = None) -> None:
        self._header.data_source.measurement.data_files.append(self._create_file(filename, timestamp, comment))

    def add_measurement_additional_file(self, filename: str, timestamp: Optional[datetime] = None, comment: Optional[str] = None) -> None:
        if self._header.data_source.measurement.additional_files is None:
            self._header.data_source.measurement.additional_files = []
        self._header.data_source.measurement.additional_files.append(self._create_file(filename, timestamp, comment))

    @staticmethod
    def _create_file(filename: str, timestamp: Optional[datetime] = None, comment: Optional[str] = None) -> File:
        return File(filename, timestamp, comment)

    def _create_mandatory_header(
        self, ws, dataset_name: str, reduction_timestamp: datetime, creator_name: str, creator_affiliation: str
    ) -> None:
        owner = Person(name=None, affiliation=None)

        run = ws.getRun()
        experiment = Experiment(
            title=None,
            instrument=ws.getInstrument().getName(),
            start_date=self._get_exp_start_time(run),
            probe=self.PROBE_NEUTRON,
        )

        sample = Sample(name=ws.getTitle())

        measurement = Measurement(instrument_settings=None, data_files=[])

        data_source = DataSource(owner=owner, experiment=experiment, sample=sample, measurement=measurement)

        software = Software(name=self.SOFTWARE_NAME, version=str(version()))
        creator = Person(name=creator_name, affiliation=creator_affiliation)
        reduction = Reduction(software=software, timestamp=reduction_timestamp, creator=creator)

        self._header = Orso(data_source, reduction, self._data_columns.header_info, dataset_name)

    def _get_exp_start_time(self, run) -> Optional[datetime]:
        if not run.hasProperty(self._RUN_START_LOG):
            return None

        try:
            return self._create_datetime_from_string(run.getProperty(self._RUN_START_LOG).value)
        except ValueError:
            return None

    @classmethod
    def _create_datetime_from_string(cls, str_datetime: str) -> datetime:
        return datetime.strptime(cls._parse_datetime_string(str_datetime), cls._DATETIME_FORMAT)

    @classmethod
    def _parse_datetime_string(cls, str_datetime: str) -> str:
        match = re.search(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}", str_datetime)
        if match is None:
            raise ValueError(f"Cannot parse datetime string {str_datetime} into required format {cls._DATETIME_FORMAT}")
        return match.group()

    @classmethod
    def create_local_datetime_from_utc_string(cls, str_utc_datetime: str) -> datetime:
        """
        Takes a datetime string in UTC and returns a datetime object in local time
        """
        utc_datetime = cls._create_datetime_from_string(str_utc_datetime)
        return utc_datetime.replace(tzinfo=timezone.utc).astimezone(tz=None)


class MantidORSODataColumns:
    # Data column units
    class Unit(Enum):
        Angstrom = "angstrom"
        InverseAngstrom = "1/angstrom"
        InverseNm = "1/nm"
        Degrees = "degrees"

    # Data column labels
    LABEL_Q = "Qz"
    LABEL_REFLECTIVITY = "R"

    # Data column physical quantity
    QUANTITY_Q = "normal_wavevector_transfer"
    QUANTITY_REFLECTIVITY = "reflectivity"

    # Error column type
    class ErrorType(Enum):
        Uncertainty = "uncertainty"
        Resolution = "resolution"

    # Error column value_is
    class ErrorValue(Enum):
        Sigma = "sigma"
        FWHM = "FWHM"

    def __init__(
        self,
        q_data: np.ndarray,
        reflectivity_data: np.ndarray,
        reflectivity_error: Optional[np.ndarray] = None,
        q_resolution: Optional[np.ndarray] = None,
        q_unit: Unit = Unit.InverseAngstrom,
        r_error_value_is: Optional[ErrorValue] = ErrorValue.Sigma,
        q_error_value_is: Optional[ErrorValue] = ErrorValue.Sigma,
    ):
        self._header_info = []
        self._data = []

        # Add the first two mandatory columns
        self._add_column(name=self.LABEL_Q, unit=q_unit, physical_quantity=self.QUANTITY_Q, data=q_data)
        self._add_column(name=self.LABEL_REFLECTIVITY, unit=None, physical_quantity=self.QUANTITY_REFLECTIVITY, data=reflectivity_data)

        # Add the third and fourth strongly recommended columns, if data is available
        if reflectivity_error is not None:
            self._add_error_column(
                error_of=self.LABEL_REFLECTIVITY, error_type=self.ErrorType.Uncertainty, value_is=r_error_value_is, data=reflectivity_error
            )
        if q_resolution is not None:
            self._add_error_column(
                error_of=self.LABEL_Q, error_type=self.ErrorType.Resolution, value_is=q_error_value_is, data=q_resolution
            )

    @property
    def header_info(self) -> List[Union[Column, ErrorColumn]]:
        return self._header_info

    @property
    def data(self) -> np.ndarray:
        return np.array(self._data).T

    def add_column(self, name: str, unit: [Unit, str], physical_quantity: str, data: np.ndarray) -> None:
        # The third and fourth strongly recommended columns are required if further columns are to be added
        self._ensure_recommended_columns_are_present(data)
        self._add_column(name, unit, physical_quantity, data)

    def add_error_column(self, error_of: str, error_type: ErrorType, value_is: ErrorValue, data: np.ndarray) -> None:
        # The third and fourth strongly recommended columns are required if further columns are to be added
        self._ensure_recommended_columns_are_present(data)
        self._add_error_column(error_of, error_type, value_is, data)

    def _add_column(self, name: str, unit: [Unit, str], physical_quantity: str, data: np.ndarray) -> None:
        unit_value = unit.value if isinstance(unit, self.Unit) else unit
        self._header_info.append(Column(name=name, unit=unit_value, physical_quantity=physical_quantity))
        self._data.append(data)

    def _add_error_column(self, error_of: str, error_type: ErrorType, value_is: Optional[ErrorValue], data: np.ndarray) -> None:
        self._header_info.append(
            ErrorColumn(error_of=error_of, error_type=error_type.value, value_is=None if value_is is None else value_is.value)
        )
        self._data.append(data)

    def _ensure_recommended_columns_are_present(self, data: np.ndarray) -> None:
        """Checks if the third and fourth strongly recommended columns are present and, if not, adds them with nan values"""

        if self._should_add_reflectivity_error_column():
            self._add_error_column(
                error_of=self.LABEL_REFLECTIVITY,
                error_type=self.ErrorType.Uncertainty,
                value_is=self.ErrorValue.Sigma,
                data=np.full(len(data), np.nan),
            )

        if self._should_add_resolution_column():
            self._add_error_column(
                error_of=self.LABEL_Q, error_type=self.ErrorType.Resolution, value_is=self.ErrorValue.Sigma, data=np.full(len(data), np.nan)
            )

    def _should_add_reflectivity_error_column(self) -> bool:
        # Check that we have only two columns and that the second is the one expected before the reflectivity error
        return (
            len(self._header_info) == 2
            and isinstance(self._header_info[1], Column)
            and self._header_info[1].name == self.LABEL_REFLECTIVITY
        )

    def _should_add_resolution_column(self) -> bool:
        # Check that we have only three columns and that the third is the one expected before the resolution
        return (
            len(self._header_info) == 3
            and isinstance(self._header_info[2], ErrorColumn)
            and self._header_info[2].error_of == self.LABEL_REFLECTIVITY
        )
