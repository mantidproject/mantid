# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
import os
import subprocess
import tempfile
from pathlib import Path
from typing import Optional
from qtpy.QtCore import QSettings

from mantid.kernel import ConfigService, ErrorReporter, Logger, UsageService
from mantid.kernel.environment import is_linux
from mantidqt.dialogs.errorreports.report import MAX_STACK_TRACE_LENGTH


class ErrorReporterPresenter(object):
    SENDING_ERROR_MESSAGE = "There was an error when sending the report.\nPlease contact mantid-help@mantidproject.org directly"

    def __init__(self, view, exit_code: str, application: str, traceback: Optional[str] = None):
        """
        :param view: A reference to the view managed by this presenter
        :param exit_code: A string containing the exit_code of the failing application
        :param application: A string containing the failing application name
        :param traceback: An optional string containing a traceback dumped as JSON-encoded string
        """
        self.error_log = Logger("errorreports")
        self._view = view
        self._exit_code = exit_code
        self._application = application
        self._traceback = traceback if traceback else ""
        self._view.set_report_callback(self.error_handler)
        self._view.moreDetailsButton.clicked.connect(self.show_more_details)

        if not traceback:
            traceback_file_path = os.path.join(ConfigService.getAppDataDirectory(), "{}_stacktrace.txt".format(application))
            try:
                if os.path.isfile(traceback_file_path):
                    with open(traceback_file_path, "r") as file:
                        self._traceback = file.readlines()
                    new_workspace_name = os.path.join(ConfigService.getAppDataDirectory(), "{}_stacktrace_sent.txt".format(application))
                    os.rename(traceback_file_path, new_workspace_name)
                elif is_linux():
                    self._traceback = self._recover_trace_from_core_dump()
            except OSError:
                pass

    def _recover_trace_from_core_dump(self):
        # TODO check core dumps are enabled?
        core_dump_dir = ConfigService.getString("errorreports.core_dumps")
        if not core_dump_dir:
            with open("/proc/sys/kernel/core_pattern") as fp:
                core_dump_dir = fp.readline().split()[0][1:]
        if not os.path.exists(core_dump_dir) or not os.path.isdir(core_dump_dir):
            return ""
        self.error_log.notice(f"Found core dump directory {core_dump_dir}")
        latest_core_dump = self._latest_core_dump(core_dump_dir)
        self.error_log.notice(f"Found latest core dump file {latest_core_dump} ({latest_core_dump.stat().st_ctime})")
        output = ""
        if latest_core_dump.suffix == ".lz4":
            output = self._decompress_lz4_then_run_gdb(latest_core_dump)
        else:
            output = self._run_gdb(latest_core_dump.as_posix())
        if not output:
            return ""
        self.error_log.notice("Trimming gdb output to extract back trace...")
        return self._trim_core_dump_file(output)

    def _decompress_lz4_then_run_gdb(self, latest_core_dump: Path):
        tmp_core_copy_fp = tempfile.NamedTemporaryFile()
        lz4_command = ["lz4", "-d", latest_core_dump.as_posix(), tmp_core_copy_fp.name]
        self.error_log.notice(f"Running {' '.join(lz4_command)} ...")
        result = subprocess.run(lz4_command, capture_output=True, text=True)
        output = ""
        if result.returncode == 0:
            self.error_log.notice(f"Decompressed core file to {tmp_core_copy_fp.name}")
            output = self._run_gdb(tmp_core_copy_fp.name)
        else:
            self.error_log.notice(f"lz4 returned non-zero exit code:\n{result.stderr}")
        tmp_core_copy_fp.close()
        return output

    def _run_gdb(self, core_file: str):
        commands_fp = tempfile.NamedTemporaryFile()
        with open(commands_fp.name, "w") as fp:
            fp.write("bt\nq\n")

        gdb_command = ["gdb", "-x", commands_fp.name, f"{os.environ['CONDA_PREFIX']}/bin/python", core_file]
        self.error_log.notice(f"Running {' '.join(gdb_command)} ...")
        result = subprocess.run(gdb_command, capture_output=True, text=True)
        commands_fp.close()
        if result.returncode == 0:
            self.error_log.notice("gdb ran successfully")
            return result.stdout
        self.error_log.notice(f"gdb returned non-zero exit code:\n{result.stderr}")
        return ""

    @staticmethod
    def _latest_core_dump(dir: str):
        # TODO check if it was created recently?
        files = Path(dir).iterdir()
        sorted_files = sorted([file for file in files], key=lambda file: file.stat().st_ctime)
        return Path(dir, sorted_files[-1])

    @staticmethod
    def _trim_core_dump_file(content: str):
        lines = content.split("\n")
        trace_begins_index = next(n for n, line in enumerate(lines) if line.startswith("#"))
        return "\n".join(lines[trace_begins_index - 1 :])

    def forget_contact_info(self):
        settings = QSettings()
        settings.beginGroup(self._view.CONTACT_INFO)
        settings.setValue(self._view.NAME, "")
        settings.setValue(self._view.EMAIL, "")
        settings.endGroup()

    def do_not_share(self, continue_working=True):
        self.error_log.notice("No information shared")
        self._handle_exit(continue_working)
        if not self._view.rememberContactInfoCheckbox.checkState():
            self.forget_contact_info()
        return -1

    def share_all_information(self, continue_working, new_name, new_email, text_box):
        uptime = UsageService.getUpTime()
        status = self._send_report_to_server(share_identifiable=True, uptime=uptime, name=new_name, email=new_email, text_box=text_box)
        self.error_log.notice("Sent full information")
        self._handle_exit(continue_working)

        # Remember name and email in QSettings
        if self._view.rememberContactInfoCheckbox.checkState():
            settings = QSettings()
            settings.beginGroup(self._view.CONTACT_INFO)
            settings.setValue(self._view.NAME, new_name)
            settings.setValue(self._view.EMAIL, new_email)
            settings.endGroup()
        else:
            self.forget_contact_info()
        return status

    def error_handler(self, continue_working, share, name, email, text_box):
        if share:
            status = self.share_all_information(continue_working, name, email, text_box)
        else:
            status = self.do_not_share(continue_working)

        self._view.close_reporter(status)

    def _handle_exit(self, continue_working):
        if not continue_working:
            self.error_log.error("Terminated by user.")
            self._view.quit()
        else:
            self.error_log.error("Continue working.")

    def _send_report_to_server(self, share_identifiable=False, name="", email="", uptime="", text_box=""):
        stacktrace = "".join(self._traceback)
        if len(stacktrace) > MAX_STACK_TRACE_LENGTH:
            difference = len(stacktrace) - MAX_STACK_TRACE_LENGTH
            stacktrace = self._cut_down_stacktrace()
            self.error_log.warning(
                f"The middle {difference+5} characters of this stack trace has been removed"
                r" and replaced with \n...\n in order to"
                f" reduce it to {MAX_STACK_TRACE_LENGTH} characters"
            )

        errorReporter = ErrorReporter(
            self._application, uptime, self._exit_code, share_identifiable, str(name), str(email), str(text_box), stacktrace
        )

        status = errorReporter.sendErrorReport()

        if status != 201:
            self._view.display_message_box(
                "Error contacting server", self.SENDING_ERROR_MESSAGE, "http request returned with status {}".format(status)
            )
            self.error_log.error("Failed to send error report http request returned status {}".format(status))

        return status

    def _cut_down_stacktrace(self):
        # server has a max size for the stack trace, if exceeded will cause an error
        stacktrace = "".join(self._traceback)
        return stacktrace[: (MAX_STACK_TRACE_LENGTH // 2 - 2)] + "\n...\n" + stacktrace[-(MAX_STACK_TRACE_LENGTH // 2 - 3) :]

    def show_view(self):
        self._view.show()

    def show_view_blocking(self):
        self._view.exec_()

    def show_more_details(self):
        error_reporter = ErrorReporter(
            self._application,
            UsageService.getUpTime(),
            self._exit_code,
            True,
            str(self._view.input_name_line_edit.text()),
            str(self._view.input_email_line_edit.text()),
            str(self._view.input_free_text.toPlainText()),
            "".join(self._traceback),
        )

        error_message_json = json.loads(error_reporter.generateErrorMessage())
        stacktrace_text = error_message_json["stacktrace"]
        del error_message_json["stacktrace"]  # remove this entry so it doesn't appear twice.
        user_information = "".join("{}: {}\n".format(key, error_message_json[key]) for key in error_message_json)
        self._view.display_more_details(user_information, stacktrace_text)
