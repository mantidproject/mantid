# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function

import os
import tokenize
import glob

import mantid.api
import mantid.kernel

_destination_file = "OutputFilepath"
_recovery_folder = "RecoveryCheckpointFolder"


class OrderWorkspaceHistory(mantid.api.PythonAlgorithm):

    def category(self):
        return "Utility"

    def name(self):
        return "OrderWorkspaceHistory"

    def summary(self):
        return "Takes a folder full of workspace histories with timestamps and" \
               " creates a single unified history from them."

    def PyInit(self):
        default_folder = os.path.join(mantid.kernel.config.getAppDataDirectory(), "recovery")
        self.declareProperty(mantid.api.FileProperty(_recovery_folder, default_folder,
                                                     action=mantid.api.FileAction.Directory),
                             "Location of all saved workspace histories")

        self.declareProperty(mantid.api.FileProperty(_destination_file,
                                                     os.path.join(default_folder, "summedHistory.py"),
                                                     action=mantid.api.FileAction.Save),
                             "File destination to write the combined history to.")

    def validateInputs(self):
        return dict()

    def PyExec(self):

        all_lines = []

        for fn in glob.iglob(os.path.join(self.getPropertyValue(_recovery_folder), '*.py')):
            with open(fn) as f:
                tokens = tokenize.generate_tokens(f.readline)
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

        # Cast as a set to remove duplicates
        unique_lines = list(set(all_lines))

        def sorting_order_workspace_history(x):
            time, _, exec_count = x[1].split()
            return time, int(exec_count)

        # Sort according to time and execCount
        unique_lines.sort(key=sorting_order_workspace_history)

        destination = self.getPropertyValue(_destination_file)

        with open(destination, 'w') as outfile:
            # Add in an extra line to import the previously implicit imports from MantidPlot
            outfile.write("from mantid.simpleapi import *\n\n")
            for x in unique_lines:
                outfile.write(x[0])

# Required to have Mantid recognise the new function
mantid.api.AlgorithmFactory.subscribe(OrderWorkspaceHistory)
