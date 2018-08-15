from __future__ import absolute_import, division, print_function

import datetime
import os
import tokenize

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
        self.write_ordered_workspace_history()

    def _concatenate_iso_datetime(self, date_time):
        '''
        Concatenates the datetime string provided by Mantid into a single integer
        Args:
            date_time: ISO 8601 standard datetime string
        Returns:
            int
        '''

        if not date_time:
            raise ValueError("No date time stamp was found in the recovery history")
        self.log().debug("Found datetime stamp: {0}".format(date_time))

        # Remove whitespace and any trailing zeros
        stipped_time = date_time.strip().rstrip('0')
        return datetime.datetime.strptime(stipped_time, "%Y-%m-%dT%H:%M:%S.%f")

    def write_ordered_workspace_history(self):
        self.log().debug("Started saving workspace histories")
        source_folder = self.getPropertyValue(_recovery_folder)

        # Get list of all workspace histories
        onlyfiles = [f for f in os.listdir(source_folder)
                     if os.path.isfile(os.path.join(source_folder, f))]
        historyfiles = [x for x in onlyfiles if x.endswith('.py')]

        self.log().debug("Found {0} history files".format(len(historyfiles)))

        all_lines = []

        for fn in historyfiles:
            with open(os.path.join(source_folder, fn)) as f:
                tokens = tokenize.generate_tokens(f.readline)

                line = None
                for t in tokens:
                # Start a new line when we see a name
                    if line == None and t[0] == tokenize.NAME:
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
        all_commands = [(''.join(t[1] for t in l[:-1]), l[-1][1][1:])
                      for l in all_lines]
        # Remove duplicate commands by casting commands as a set
        unique_commands = set()
        for command in all_commands:
            unique_commands.add(command)

        # Sort the new list on datetime integer
        unique_commands = list(unique_commands)
        unique_commands.sort(key=lambda time: (time[1]))

        destination = self.getPropertyValue(_destination_file)

        self.log().debug("Writing commands to file")
        # Write to file
        with open(destination, 'w') as outfile:
            for x in unique_commands:
                outfile.write('{} # {} \n'.format(x[0], x[1]))


# Required to have Mantid recognise the new function
mantid.api.AlgorithmFactory.subscribe(OrderWorkspaceHistory)
