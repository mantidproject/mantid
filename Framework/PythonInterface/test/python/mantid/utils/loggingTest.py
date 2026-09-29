# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.kernel import ConfigService, logger
from mantid.utils.logging import capture_logs, log_to_python
from testhelpers import temporary_config

# standard imports
import logging
import subprocess
import sys
import time
import unittest


class CaptureHandler(logging.Handler):
    def __init__(self):
        super().__init__(level=logging.DEBUG)
        self.records = []

    def emit(self, record: logging.LogRecord):
        self.records.append(record)


LIVE_DATA_GIL_REGRESSION_SCRIPT = r"""
import logging
import os
import sys
import time

from mantid.api import AlgorithmManager
from mantid.kernel import ConfigService
from mantid.simpleapi import StartLiveData
from mantid.utils.logging import log_to_python as mtd_log_to_python


class CaptureHandler(logging.Handler):
    def __init__(self):
        super().__init__(level=logging.NOTSET)
        self.records = []

    def emit(self, record):
        self.records.append(record.getMessage())


ConfigService.updateFacilities(os.path.join(ConfigService.getInstrumentDirectory(), "unit_testing/UnitTestFacilities.xml"))
ConfigService.setFacility("TEST")

handler = CaptureHandler()
mantid_logger = logging.getLogger("Mantid")
mantid_logger.setLevel(logging.INFO)
mantid_logger.addHandler(handler)
mtd_log_to_python(level="notice", pattern="%t")

StartLiveData(Instrument="FakeEventDataListener", OutputWorkspace="live", UpdateEvery=0.05)

startup_deadline = time.monotonic() + 5.0
while not any(message.startswith("MonitorLiveData started") for message in handler.records):
    if time.monotonic() > startup_deadline:
        raise RuntimeError("MonitorLiveData did not start")
    time.sleep(0.01)

old_switch_interval = sys.getswitchinterval()
hold_started = time.monotonic()
try:
    # Prevent the main Python thread from periodically yielding the GIL.
    sys.setswitchinterval(10.0)
    hold_deadline = hold_started + 1.0
    while time.monotonic() < hold_deadline:
        pass
finally:
    sys.setswitchinterval(old_switch_interval)

chunk_messages = [message for message in handler.records if message.startswith("Loading live data chunk")]
if not chunk_messages:
    raise RuntimeError("MonitorLiveData did not progress while Python held the GIL")
last_chunk = int(chunk_messages[-1].split()[4])
if last_chunk < 2:
    raise RuntimeError(f"MonitorLiveData only reached chunk {last_chunk} while Python held the GIL")

AlgorithmManager.cancelAll()
cleanup_deadline = time.monotonic() + 5.0
while AlgorithmManager.runningInstancesOf("MonitorLiveData"):
    if time.monotonic() > cleanup_deadline:
        raise RuntimeError("MonitorLiveData did not stop")
    time.sleep(0.01)
time.sleep(0.1)
AlgorithmManager.shutdown()
"""


class loggingTest(unittest.TestCase):
    def test_capture_logs(self):
        with capture_logs() as logs:
            logger.error("Error message")
            self.assertTrue("Error message" in logs.getvalue())

        with temporary_config():
            config = ConfigService.Instance()
            config["logging.loggers.root.level"] = "information"
            with capture_logs(level="error") as logs:
                self.assertTrue(config["logging.loggers.root.level"] == "error")
                logger.error("Error-message")
                logger.debug("Debug-message")
                self.assertTrue("Error-message" in logs.getvalue())
                self.assertFalse("Debug-message" in logs.getvalue())

            self.assertTrue(config["logging.loggers.root.level"] == "information")

    def test_log_to_python(self):
        py_logger = logging.getLogger("Mantid")
        py_logger.setLevel(logging.INFO)
        handler = CaptureHandler()
        for hdlr in py_logger.handlers:
            py_logger.removeHandler(hdlr)
        py_logger.addHandler(handler)

        with temporary_config():
            log_to_python(level="information", pattern="%t")
            logger.information("[[info]]")
            logger.warning("[[warning]]")
            logger.error("[[error]]")
            logger.fatal("[[fatal]]")

        deadline = time.monotonic() + 5.0
        while len(handler.records) < 4 and time.monotonic() < deadline:
            time.sleep(0.01)

        self.assertListEqual([record.msg for record in handler.records], ["[[info]]", "[[warning]]", "[[error]]", "[[fatal]]"])
        self.assertListEqual([record.levelname for record in handler.records], ["INFO", "WARNING", "ERROR", "CRITICAL"])

        py_logger.removeHandler(handler)

    def test_monitor_live_data_progresses_while_python_holds_the_gil(self):
        # Isolate the long-lived MonitorLiveData thread and bound any deadlock during cleanup.
        result = subprocess.run(
            [sys.executable, "-c", LIVE_DATA_GIL_REGRESSION_SCRIPT], capture_output=True, text=True, timeout=15, check=False
        )
        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")


if __name__ == "__main__":
    unittest.main()
