# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import AlgorithmFactory, AlgorithmManager, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty


class AddSampleLogMultiple(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Logs"

    def summary(self):
        return "Add multiple sample logs to a workspace"

    def seeAlso(self):
        return ["AddSampleLog"]

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("Workspace", "", direction=Direction.InOut), doc="Workspace to add logs to")

        self.declareProperty(StringArrayProperty("LogNames", ""), doc="Comma separated list of log names")

        self.declareProperty(StringArrayProperty("LogValues", ""), doc="Comma separated list of log values")

        self.declareProperty(StringArrayProperty("LogUnits", ""), doc="Comma separated list of log units")

        self.declareProperty("ParseType", True, doc="Determine the value type by parsing the string")

        self.declareProperty(StringArrayProperty("LogTypes", ""), doc="Comma separated list of types the log values will be.")

    def PyExec(self):
        workspace = self.getProperty("Workspace").value
        log_names = self.getProperty("LogNames").value
        log_values = self.getProperty("LogValues").value
        log_units = self.getProperty("LogUnits").value
        parse_type = self.getProperty("ParseType").value
        log_types = self.getProperty("LogTypes").value

        if len(log_units) == 0:
            log_units = [""] * len(log_names)

        for idx in range(0, len(log_names)):
            # Get the name, value, and unit
            name = log_names[idx]
            value = log_values[idx]
            unit = log_units[idx]

            # Try to get the correct type
            value_type = "String"
            if parse_type:
                try:
                    float(value)
                    value_type = "Number"
                except ValueError:
                    pass
            elif len(log_types):
                value_type = log_types[idx]

            # Add the log
            alg = AlgorithmManager.create("AddSampleLog")
            alg.initialize()
            alg.setChild(True)
            alg.setLogging(False)
            alg.setProperty("Workspace", workspace)
            alg.setProperty("LogType", value_type)
            alg.setProperty("LogName", name)
            alg.setProperty("LogText", value)
            alg.setProperty("LogUnit", unit)
            alg.execute()

    def validateInputs(self):
        issues = dict()

        log_names = self.getProperty("LogNames").value
        log_values = self.getProperty("LogValues").value
        log_units = self.getProperty("LogUnits").value
        log_types = self.getProperty("LogTypes").value

        num_names = len(log_names)
        num_values = len(log_values)
        num_units = len(log_units)
        num_types = len(log_types)

        # Ensure there is at least 1 log name
        if num_names == 0:
            issues["LogNames"] = "Must have at least one log name"

        # Ensure there is at least 1 log value
        if num_values == 0:
            issues["LogValues"] = "Must have at least one log value"

        if num_names > 0 and num_values > 0 and num_names != num_values:
            issues["LogValues"] = "Number of log values must match number of log names"

        if num_names > 0 and num_units != 0 and num_units != num_names:
            issues["LogUnits"] = "Number of log units must be 0 or match the number of log names"

        if self.getProperty("ParseType").value and num_types != 0:
            issues["LogTypes"] = "LogTypes array not used when ParseType=True"

        if num_names > 0 and num_types != 0 and num_types != num_names:
            issues["LogTypes"] = "Number of log types must be 0 or match the number of log names"

        allowed_types = ["String", "Number", "Number Series"]
        for value in log_types:
            if value not in allowed_types:
                issues["LogTypes"] = "{} is not an allowed log type".format(value)

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(AddSampleLogMultiple)
