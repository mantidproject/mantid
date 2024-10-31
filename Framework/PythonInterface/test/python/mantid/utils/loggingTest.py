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
import unittest


class CaptureHandler(logging.Handler):
    def __init__(self):
        super().__init__(level=logging.DEBUG)
        self.records = []

    def emit(self, record: logging.LogRecord):
        self.records.append(record)


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
            log_to_python()
            logger.information("[[info]]")
            logger.warning("[[warning]]")
            logger.error("[[error]]")
            logger.fatal("[[fatal]]")

        self.assertListEqual([record.msg for record in handler.records], ["[[info]]", "[[warning]]", "[[error]]", "[[fatal]]"])
        self.assertListEqual([record.levelname for record in handler.records], ["INFO", "WARNING", "ERROR", "CRITICAL"])

        py_logger.removeHandler(handler)


if __name__ == "__main__":
    unittest.main()
