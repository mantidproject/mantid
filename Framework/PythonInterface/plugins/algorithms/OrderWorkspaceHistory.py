from __future__ import absolute_import, division, print_function

import datetime
import os

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

        # Read each history in as a list of tuples
        original_lines = set()
        for infile in historyfiles:
            with open(os.path.join(source_folder, infile), 'r') as f:
                for line in f:
                    original_lines.add(line)

        commands = []
        # Remove any comment lines
        for line in original_lines:
            stripped_line = line.lstrip()
            if stripped_line[0] is not '#':
                commands.append(stripped_line)

        self.log().debug("Found {0} unique commands".format(len(commands)))

        # Add lists of histories together
        all_unique_commands = [command.strip().split('#') for command in commands]

        # Convert the datetime into a sortable integer
        all_unique_commands = [(i[0], self._concatenate_iso_datetime(i[1]))
                               for i in all_unique_commands]

        # Sort the new list on datetime integer
        all_unique_commands.sort(key=lambda time: (time[1]))

        destination = self.getPropertyValue(_destination_file)

        self.log().debug("Writing commands to file")
        # Write to file
        with open(destination, 'w') as outfile:
            for x in all_unique_commands:
                outfile.write('{} # {} \n'.format(x[0], x[1]))


# Required to have Mantid recognise the new function
mantid.api.AlgorithmFactory.subscribe(OrderWorkspaceHistory)
