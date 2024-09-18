"""This test is to verify a fix for a deadlock that occurs when using
the PythonStdoutChannel or PythonLoggingChannel Poco log channels and
when the CheckMantidVersion or DownloadInstrument algorithms are run
during Mantid Framework creation.

The deadlock involves these logging channels first locking the mutex
in Poco::SplitterChannel::log before trying to acquire the Python
GIL. But the Python GIL is being currently locked by the main thread
which is then trying to acquire the mutex in
Poco::SplitterChannel::log.

If any of these test raise a subprocess.TimeoutExpired error that
means it has entered a deadlock.

A python script is created and run in a separate process so that the
`import mantid.simpleapi` can be tested correctly.

This test stands up a http server to serve the request form these
algorithms to avoid hitting the internet.

"""

import os
import sys
import time
import unittest
import subprocess
import systemtesting
import platform
from multiprocessing import Process
from http.server import HTTPServer, BaseHTTPRequestHandler

script_template = """
import time
from mantid.kernel import ConfigService
from mantid.api import FrameworkManagerImpl

# If the framework has already been created then this test is invalid.
if FrameworkManagerImpl.hasInstance():
    exit(1)

ConfigService["logging.loggers.root.level"] = "debug"
ConfigService["logging.channels.consoleChannel.class"] = "{LoggingClass}"

ConfigService["CheckMantidVersion.GitHubReleaseURL"]="http://localhost:18888/version"
ConfigService["UpdateInstrumentDefinitions.URL"]="http://localhost:18888/instruments"

ConfigService["CheckMantidVersion.OnStartup"] = "{CheckMantidVersion}"
ConfigService["UpdateInstrumentDefinitions.OnStartup"] = "{UpdateInstrumentDefinitions}"

import mantid.simpleapi
"""

TEST_SCRIPT_NAME = "pythonloggingtestscript.py"


def run_server():
    class Handler(BaseHTTPRequestHandler):
        def _set_headers(self):
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()

        def do_GET(self):
            time.sleep(0.1)
            self._set_headers()
            if self.path == "/instruments":
                self.wfile.write("[]".encode())
            else:
                self.wfile.write('{"tag_name": "v6.8.0"}'.encode())

    HTTPServer(("", 18888), Handler).serve_forever()


class PythonLoggingTests(unittest.TestCase):
    def test_PythonStdoutChannel_check_mantid_version(self):
        self._create_script_and_execute(LoggingClass="PythonStdoutChannel", CheckMantidVersion=1)

    def test_PythonLoggingChannel_check_mantid_version(self):
        self._create_script_and_execute(LoggingClass="PythonLoggingChannel", CheckMantidVersion=1)

    def test_PythonStdoutChannel_update_instruments(self):
        self._create_script_and_execute(LoggingClass="PythonStdoutChannel", UpdateInstrumentDefinitions=1)

    def test_PythonLoggingChannel_update_instruments(self):
        self._create_script_and_execute(LoggingClass="PythonLoggingChannel", UpdateInstrumentDefinitions=1)

    def _create_script_and_execute(self, LoggingClass="PythonStdoutChannel", CheckMantidVersion=0, UpdateInstrumentDefinitions=0):
        with open(TEST_SCRIPT_NAME, "w") as f:
            f.write(
                script_template.format(
                    LoggingClass=LoggingClass,
                    CheckMantidVersion=CheckMantidVersion,
                    UpdateInstrumentDefinitions=UpdateInstrumentDefinitions,
                )
            )

        result = subprocess.run([sys.executable, TEST_SCRIPT_NAME], timeout=60)  # This will raise subprocess.TimeoutExpired if deadlocke
        result.check_returncode()


class PythonChannelTest(systemtesting.MantidSystemTest):
    def cleanup(self):
        self._server.terminate()
        os.remove(TEST_SCRIPT_NAME)

    def skipTests(self):
        # skip if OSX because multiprocessing uses `spawn` instead of `fork` which causes this test to fail
        return platform.system() == "Darwin"

    def runTest(self):
        self._server = Process(target=run_server)
        self._server.start()
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(PythonLoggingTests, "test"))
        runner = unittest.TextTestRunner()
        try:
            self.assertTrue(runner.run(suite).wasSuccessful())
        except:
            # Make sure cleanup does run if exception raised
            self.cleanup()
            raise
