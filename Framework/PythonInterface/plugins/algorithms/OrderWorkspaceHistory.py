from __future__ import absolute_import, division, print_function

import datetime
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
        self.log().debug("Started saving workspace histories")

        all_lines = []

        #for fn in historyfiles:
        for fn in glob.iglob(os.path.join(self.getPropertyValue(_recovery_folder), '*.py')):
            with open(fn) as f:
                tokens = tokenize.generate_tokens(f.readline)

                line = None
                for t in tokens:
                    #Start a new line when we see a name
                    if line is None and t[0] == tokenize.NAME:
                        line = [t]
                    # End the line when we see a logical line ending
                    elif t[0] == tokenize.NEWLINE:
                        # Only care about the line if it has a comment
                        have_comment = any(x[0] == tokenize.COMMENT for x in line)
                        if have_comment:
                            all_lines.append(line)
                        line = []
                    # Everything in between we care about
                    elif line is not None:
                        line.append(t)

        # l[-1][1][1:] is the comment string, with the preceeding hash stripped off
        all_commands = [(''.join(t[1] for t in l[:-1]), l[-1][1][1:]) for l in all_lines]
        # Remove duplicate commands by casting commands as a set
        unique_commands = list(set(all_commands))

        unique_commands.sort(key=lambda time: (time[1]))

        destination = self.getPropertyValue(_destination_file)

        self.log().debug("Writing commands to file")
        # Write to file
        with open(destination, 'w') as outfile:
            for x in unique_commands:
                outfile.write('{} # {} \n'.format(x[0], x[1]))

# Required to have Mantid recognise the new function
mantid.api.AlgorithmFactory.subscribe(OrderWorkspaceHistory)
