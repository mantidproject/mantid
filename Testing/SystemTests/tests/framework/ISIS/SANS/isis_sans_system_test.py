# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import pathlib

from mantid import config
from SANS.sans.common.enums import SANSInstrument


# Decorator to fix-up path to files
def ISISSansSystemTest(*args: SANSInstrument):
    def class_decorator(cls):
        setUp = getattr(cls, "setUp")
        tearDown = getattr(cls, "tearDown")

        _instruments = [i for i in args]
        _config_before: str = config["datasearch.directories"]

        def patched_setUp(*args):
            # This relies on the assumption that system tests are first in the search dirs
            # which is also assumed in several other system tests
            for i in _instruments:
                _inst_dir = pathlib.Path(_config_before.split(";")[0]) / "ISIS_SANS" / i.value
                config["datasearch.directories"] += ";" + str(_inst_dir.resolve())
            setUp(*args)  # Call original

        def patched_tearDown(*args):
            config["datasearch.directories"] = _config_before
            tearDown(*args)  # Call original

        # And now we patch
        setattr(cls, "setUp", patched_setUp)
        setattr(cls, "tearDown", patched_tearDown)
        return cls

    return class_decorator
