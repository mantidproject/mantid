from __future__ import absolute_import, division, print_function

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
        date = date_time.split('T')[0]
        time = date_time.split('T')[1]
        yy = str(date.split('-')[0])
        mo = str(date.split('-')[1])
        dd = str(date.split('-')[2])
        hh = str(time.split(':')[0])
        mm = str(time.split(':')[1])
        secs = time.split(':')[2]
        ss = str(secs.split('.')[0])
        ms = str(secs.split('.')[1])

        return int(yy + mo + dd + hh + mm + ss + ms)

    def write_ordered_workspace_history(self):
        self.log().debug("Started saving workspace histories")
        source_folder = self.getPropertyValue(_recovery_folder)

        # Get list of all workspace histories
        onlyfiles = [f for f in os.listdir(source_folder)
                     if os.path.isfile(os.path.join(source_folder, f))]
        historyfiles = [x for x in onlyfiles if x.endswith('.py')]

        self.log().debug("Found {0} history files".format(len(historyfiles)))

        # Read each history in as a list of tuples
        commands = set()
        for infile in historyfiles:
            with open(os.path.join(source_folder, infile), 'r') as f:
                for line in f:
                    commands.add(line)

        self.log().debug("Found {0} unique commands".format(len(commands)))

        # Add lists of histories together
        all_unique_commands = [command.strip().split('#') for command in commands]

        self.log().debug("1")

        # Convert the datetime into a sortable integer
        all_unique_commands = [(i[0], self._concatenate_iso_datetime(i[1]))
                               for i in all_unique_commands]

        self.log().debug("2")

        # Sort the new list on datetime integer
        all_unique_commands.sort(key=lambda time: (time[1]))

        self.log().debug("3")

        destination = self.getPropertyValue(_destination_file)

        self.log().debug("4")

        self.log().debug("Writing commands to file")
        # Write to file
        with open(destination, 'w') as outfile:
            for x in all_unique_commands:
                outfile.write('{} # {} \n'.format(x[0], x[1]))


# Required to have Mantid recognise the new function
mantid.api.AlgorithmFactory.subscribe(OrderWorkspaceHistory)
