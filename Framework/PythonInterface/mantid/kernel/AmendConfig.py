# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from contextlib import contextmanager
from copy import deepcopy
from typing import Union

from mantid.kernel import ConfigService
from . import logger


@contextmanager
def amend_config(
    facility: str = None, instrument: str = None, data_dir: Union[str, list] = None, prepend_datadir: bool = True, **kwargs
) -> None:
    r"""

    Context manager to safely modify :ref:`Mantid Configuration Service <Config Service>` while
    the function is executed.


    facility : string
        Sets the value for :ref:`default.facility <Facility Properties>` *Changing the facility also changes the instrument
        to the default instrument for the facility. It is recommended to provide an instrument with a facility*
    instrument : string
        Sets the value for :ref:`default.instrument <Facility Properties>`
    data_dir : string, list
        Sets the value for :ref:`datasearch.directories <Directory Properties>`
    prepend_datadir : bool
        Default `True`, if `False` the ``datasearch.directories`` will be replaced with the value of ``data_dir``
    kwargs
        Dictionary of any named keyword arguments from :ref:`Mantid Properties <Properties File>`


    **Example Usage**


    .. code-block:: python

        from mantid.kernel import amend_config

        with amend_config(facility="FacilityA", instrument="InstrumentX"):
            # Inside this block, configuration for FacilityA and InstrumentX is active.
            perform_custom_operation()

        # Outside the block, the original configuration is restored.

        with amend_config(data_dir="/custom/data"):
            # Configuration with a custom data directory.
            do_something_with_custom_data()

        # Original configuration is restored again.

        temp_config = {"Notifications.Enabled": "On", "logging.loggers.root.level": "debug"}
        with amend_config(**temp_config):
            # Configuration with dictionary of properties
            do_something_else()

        # Original configuration is restored again.
    """
    modified_keys = list()
    backup = dict()
    config = ConfigService.Instance()
    DEFAULT_FACILITY = "default.facility"
    DEFAULT_INSTRUMENT = "default.instrument"
    SEARCH_ARCHIVE = "datasearch.searcharchive"
    DATASEARCH_DIR = "datasearch.directories"

    backup[DEFAULT_FACILITY] = config[DEFAULT_FACILITY]
    backup[DEFAULT_INSTRUMENT] = config[DEFAULT_INSTRUMENT]

    if facility:
        config.setFacility(facility)
        modified_keys.append(DEFAULT_FACILITY)
        modified_keys.append(DEFAULT_INSTRUMENT)
    logger.information(f"kernel.amend_config: using default.facility {config[DEFAULT_FACILITY]}")

    if instrument:
        config[DEFAULT_INSTRUMENT] = instrument
        if DEFAULT_INSTRUMENT not in modified_keys:
            modified_keys.append(DEFAULT_INSTRUMENT)
    logger.information(f"kernel.amend_config: using default.instrument {config[DEFAULT_INSTRUMENT]}")

    if data_dir is not None:
        data_dirs = (
            [
                data_dir,
            ]
            if isinstance(data_dir, str)
            else data_dir
        )
        backup[DATASEARCH_DIR] = deepcopy(config[DATASEARCH_DIR])
        modified_keys.append(DATASEARCH_DIR)
        # Prepend or replace data search dirs
        if prepend_datadir:
            config.setDataSearchDirs(data_dirs + list(config.getDataSearchDirs()))
        else:
            config.setDataSearchDirs(data_dirs)

    if kwargs is not None:
        if SEARCH_ARCHIVE not in kwargs:
            kwargs[SEARCH_ARCHIVE] = "hfir, sns"
        for key, val in kwargs.items():
            backup[key] = config[key]
            config[key] = val
            modified_keys.append(key)

    try:
        yield
    finally:
        for key in modified_keys:
            config[key] = backup[key]
