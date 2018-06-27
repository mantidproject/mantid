from __future__ import print_function
import csv
import os
from os import listdir
from os.path import isfile, join

from mantid.api import IFunctionGeneral, FunctionFactory


class OrderWorkspaceHistory(IFunctionGeneral):
    def __init__(self):
        self._recovery_folder = "RecoveryCheckpointFolder"
        self._destination_file = "OutputFilepath"

    def init(self):
        default_folder = os.path.join(config.getAppDataDirectory(), "recovery")
        self.declareParameter(self._recovery_folder, default_folder,
                              "Location of all saved workspace histories")

    @staticmethod
    def _concatenate_iso_datetime(date_time):
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
        source_folder = self.getParameterValue(self._recovery_folder)

        # Get list of all workspace histories
        onlyfiles = [f for f in listdir(source_folder) if isfile(join(source_folder, f))]
        historyfiles = [x for x in onlyfiles if x.endswith('.py')]

        # Read each history in as a list of tuples
        commands = set()
        for infile in historyfiles:
            with open(infile) as f:
                for line in f:
                    commands.add(line)

        # Add lists of histories together
        all_unique_commands = list(commands)

        # Convert the datetime into a sortable integer
        all_unique_commands = [(i[0],
                                OrderWorkspaceHistory._concatenate_iso_datetime(i[1]))
                               for i in all_unique_commands]

        # Sort the new list on datetime integer
        all_unique_commands.sort(key=lambda time: (time[1]))

        destination = self.getParameterValue(self._destination_file)

        # Write to file
        with open(destination, 'w') as outfile:
            for x in all_unique_commands:
                outfile.write('{} # {} \n'.format(x[0], x[1]))


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(OrderWorkspaceHistory)