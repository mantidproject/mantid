# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tokenize
import glob

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

import mantid.api
import mantid.kernel
from mantid.kernel import Direction

_destination_file = "OutputFilepath"
_recovery_folder = "RecoveryCheckpointFolder"
_default_folder = os.path.join(mantid.kernel.config.getAppDataDirectory(), "recovery")


class OrderWorkspaceHistory(mantid.api.PythonAlgorithm):
    def category(self):
        return "Utility"

    def name(self):
        return "OrderWorkspaceHistory"

    def summary(self):
        return "Takes a folder full of workspace histories or a string with timestamps and creates a single unified history from them."

    def PyInit(self):
        self.declareProperty(
            mantid.api.FileProperty(_recovery_folder, _default_folder, action=mantid.api.FileAction.OptionalDirectory),
            "Location of all saved workspace histories",
        )

        self.declareProperty(
            mantid.api.FileProperty(
                _destination_file, os.path.join(_default_folder, "summedHistory.py"), action=mantid.api.FileAction.OptionalSave
            ),
            "File destination to write the combined history to.",
        )

        self.declareProperty("InputString", "", "The string to be turned into a single cohesive history", Direction.Input)
        self.declareProperty("OutputString", "", "The output from the algorithm when InputString is given", Direction.Output)

    def validateInputs(self):
        dic = {}
        if self.getPropertyValue("InputString") != "" and self.getPropertyValue(_recovery_folder) != _default_folder:
            dic["InputString"] = "InputString provided when recoveryFolder also provided"
        return dic

    @staticmethod
    def _get_all_lines_from_io(readline):
        all_lines = []
        tokens = tokenize.generate_tokens(readline)
        line = None
        for t in tokens:
            # Start a new line when we see a name
            if line is None and t[0] == tokenize.NAME:
                line = [t]
            # End the line when we see a logical line ending
            elif t[0] == tokenize.NEWLINE:
                # Only care about the line if it has a comment
                have_comment = any(x[0] == tokenize.COMMENT for x in line)
                if have_comment:
                    # line[-1][1][1:] is the comment string, with the preceeding hash stripped off;
                    # line[0][4] is the command and the comment
                    all_lines.append((line[-1][4], line[-1][1][1:]))
                line = []
            # Everything in between we care about
            elif line is not None:
                line.append(t)
        return all_lines

    def PyExec(self):
        all_lines = []
        input_string = self.getPropertyValue("InputString")

        if input_string == "":
            for fn in glob.iglob(os.path.join(self.getPropertyValue(_recovery_folder), "*.py")):
                with open(fn) as f:
                    all_lines.extend(self._get_all_lines_from_io(f.readline))
        else:
            # In all scenarios we need a newline character at the end of a line however algorithm properties strip
            # whitespace characters
            input_string += "\n"
            script_string_io = StringIO(input_string)
            all_lines = self._get_all_lines_from_io(script_string_io.readline)

        # Cast as a set to remove duplicates
        unique_lines = list(set(all_lines))

        def sorting_order_workspace_history(x):
            time, _, exec_count = x[1].split()
            return time, int(exec_count)

        # Sort according to time and execCount
        unique_lines.sort(key=sorting_order_workspace_history)

        destination = self.getPropertyValue(_destination_file)

        script = ""
        for line in unique_lines:
            script += line[0]

        if input_string == "":
            with open(destination, "w") as outfile:
                # Add in an extra line to import the previously implicit imports from Mantid
                outfile.write("from mantid.simpleapi import *\n\n")
                outfile.write(script)
        else:
            self.setPropertyValue("OutputString", script)


# Required to have Mantid recognise the new function
mantid.api.AlgorithmFactory.subscribe(OrderWorkspaceHistory)
