#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class AddSampleLogMultiple(PythonAlgorithm):

    def category(self):
        return 'DataHandling\\Logs'


    def summary(self):
        return 'Add multiple sample logs to a workspace'


    def PyInit(self):
        self.declareProperty(WorkspaceProperty('Workspace', '', direction=Direction.InOut),
                doc='Workspace to add logs to')

        self.declareProperty(StringArrayProperty('LogNames', ''),
                             doc='Comma separated list of log names')

        self.declareProperty(StringArrayProperty('LogValues', ''),
                             doc='Comma separated list of log values')

        self.declareProperty('ParseType', True,
                             doc='Determine the value type by parsing the string')


    def PyExec(self):
        workspace = self.getPropertyValue('Workspace')
        log_names = self.getProperty('LogNames').value
        log_values = self.getProperty('LogValues').value
        parse_type = self.getProperty('ParseType').value

        for idx in range(0, len(log_names)):
            # Get the name and value
            name = log_names[idx]
            value = log_values[idx]

            # Try to get the correct type
            value_type = 'String'
            if parse_type:
                try:
                    float(value)
                    value_type = 'Number'
                except ValueError:
                    pass

            # Add the log
            alg = AlgorithmManager.create('AddSampleLog')
            alg.initialize()
            alg.setChild(True)
            alg.setLogging(False)
            alg.setProperty('Workspace', workspace)
            alg.setProperty('LogType', value_type)
            alg.setProperty('LogName', name)
            alg.setProperty('LogText', value)
            alg.execute()


    def validateInputs(self):
        issues = dict()

        log_names = self.getProperty('LogNames').value
        log_values = self.getProperty('LogValues').value

        num_names = len(log_names)
        num_values = len(log_values)

        # Ensure there is at leats 1 log name
        if num_names == 0:
            issues['LogNames'] = 'Must have at least one log name'

        # Ensure there is at leats 1 log value
        if num_values == 0:
            issues['LogValues'] = 'Must have at least one log value'

        if num_names > 0 and num_values > 0 and num_names != num_values:
            issues['LogValues'] = 'Number of log values must match number of log names'

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(AddSampleLogMultiple)
