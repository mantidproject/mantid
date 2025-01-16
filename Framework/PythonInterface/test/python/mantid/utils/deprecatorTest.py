# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from io import StringIO

from mantid.api import PythonAlgorithm
from mantid.kernel import ConfigService, logger
from mantid.utils.deprecator import deprecated_algorithm

# standard imports
import sys
import unittest


@deprecated_algorithm("MyNewAlg", "2020-12-25")
class MyOldAlg(PythonAlgorithm):
    def category(self):
        return "Inelastic\\Reduction"

    def PyInit(self):
        self.declareProperty("Meaning", 42, "Assign a meaning to the Universe")

    def PyExec(self):
        logger.notice(f"The meaning of the Universe is {self.getPropertyValue('Meaning')}")


class RedirectStdOut:
    r"""redirect logging messages to a file"""

    # Adapted from https://stackoverflow.com/a/45899925

    CONFIG_KEY = "logging.channels.consoleChannel.class"

    def __init__(self):
        self._stdout = None
        self._string_io = None
        self._original_config = None
        self._config = ConfigService.Instance()

    def __enter__(self):
        self._original_config = self._config[self.CONFIG_KEY]
        self._stdout = sys.stdout
        sys.stdout = self._string_io = StringIO()
        # redirect log messages to log_file
        self._config[self.CONFIG_KEY] = "PythonStdoutChannel"
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        sys.stdout = self._stdout
        self._config[self.CONFIG_KEY] = self._original_config

    def __str__(self):
        return self._string_io.getvalue()


class DeprecatorTest(unittest.TestCase):
    @staticmethod
    def _exec_alg(log_type: str) -> str:
        config = ConfigService.Instance()
        original_conf = config["algorithms.deprecated"]
        with RedirectStdOut() as log_file:
            alg = MyOldAlg()
            alg.initialize()
            alg.setProperty("Meaning", 42)
            config["algorithms.deprecated"] = log_type
            alg.execute()

        config["algorithms.deprecated"] = original_conf
        return str(log_file)

    def test_deprecated_algorithm_raise(self):
        log_file = self._exec_alg("raise")
        self.assertTrue("Error in execution of algorithm MyOldAlg" in log_file)

    def test_deprecated_algorithm_log(self):
        log_file = self._exec_alg("log")
        self.assertTrue(' Algorithm "MyOldAlg" is deprecated' in log_file)


if __name__ == "__main__":
    unittest.main()
