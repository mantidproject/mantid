# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceGroup, MatrixWorkspace
from mantid.kernel import Direction, StringArrayLengthValidator, StringArrayProperty, FloatBoundedValidator
import mantid.simpleapi as api
import numpy as np


class CompareSampleLogs(PythonAlgorithm):
    """
    Compares specified sample logs for a given list of workspaces
    """

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.tolerance = 0.0
        self.wslist = []

    def category(self):
        """
        Returns category
        """
        return "Utility\\Workspaces"

    def seeAlso(self):
        return ["CompareWorkspaces", "CheckForSampleLogs", "CopySample"]

    def name(self):
        """
        Returns name
        """
        return "CompareSampleLogs"

    def summary(self):
        return "Compares specified sample logs for a given list of workspaces."

    def PyInit(self):
        validator = StringArrayLengthValidator()
        validator.setLengthMin(1)  # one group may be given
        self.declareProperty(
            StringArrayProperty(name="InputWorkspaces", direction=Direction.Input, validator=validator),
            doc="Comma separated list of workspaces or groups of workspaces.",
        )

        self.declareProperty(
            StringArrayProperty(name="SampleLogs", direction=Direction.Input, validator=validator),
            doc="Comma separated list of sample logs to compare.",
        )
        self.declareProperty(
            "Tolerance", 1e-3, validator=FloatBoundedValidator(lower=1e-7), doc="Tolerance for comparison of numeric values."
        )
        self.declareProperty(
            "Result",
            "A string that will be empty if all the logs match, otherwise will contain a comma separated list of  not matching logs",
            Direction.Output,
        )

        return

    def validateInputs(self):
        # given workspaces must exist
        # and must be public of ExperimentInfo
        issues = dict()
        workspaces = self.getProperty("InputWorkspaces").value
        for wsname in workspaces:
            if not api.AnalysisDataService.doesExist(wsname):
                issues["InputWorkspaces"] = "Workspace " + wsname + " does not exist."
            else:
                wks = api.AnalysisDataService.retrieve(wsname)
                if isinstance(wks, WorkspaceGroup):
                    for idx in range(wks.getNumberOfEntries()):
                        if not isinstance(wks.getItem(idx), MatrixWorkspace):
                            issues["InputWorkspaces"] = "Group " + wsname + " contains workspaces of unsupported type."
                elif not isinstance(wks, MatrixWorkspace):
                    issues["InputWorkspaces"] = "Type of workspace " + wsname + " is not supported by this algorithm."

        return issues

    def _expand_groups(self):
        workspaces = self.getProperty("InputWorkspaces").value
        input_workspaces = []
        for wsname in workspaces:
            wks = api.AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                input_workspaces.extend(wks.getNames())
            else:
                input_workspaces.append(wsname)

        return input_workspaces

    def do_match(self, pname, properties, isnum):
        wsnum = len(self.wslist)
        nprop = len(properties)
        # if some workspaces do not have this property, return False
        if nprop != wsnum:
            message = (
                "Number of properties " + str(nprop) + " for property " + pname + " is not equal to number of workspaces " + str(wsnum)
            )
            self.log().information(message)
            return False

        match = True
        if isnum:
            # check for nan
            idx_nan = np.where(np.isnan(properties))[0]
            if len(idx_nan) > 0:
                message = (
                    "Sample log "
                    + pname
                    + " contains nan values. \n"
                    + "Workspaces: "
                    + ", ".join(self.wslist)
                    + "\n Values: "
                    + str(properties)
                )
                self.log().information(message)
                return False

            if max(properties) - min(properties) > self.tolerance:
                match = False
        else:
            pvalue = properties[0]
            if properties.count(pvalue) != nprop:
                match = False
        if not match:
            message = (
                "Sample log "
                + pname
                + " is not identical in the given list of workspaces. \n"
                + "Workspaces: "
                + ", ".join(self.wslist)
                + "\n Values: "
                + str(properties)
            )
            self.log().information(message)
        return match

    def compare_properties(self, plist):
        """
        Compares properties which are required to be the same.
        Produces error message and throws exception if difference is observed
        or if one of the sample logs is not found.
        Important: exits after the first difference is observed. No further check is performed.
            @param plist   List of properties to compare
        """
        # retrieve the workspaces, form dictionary {wsname: run}
        runs = {}
        does_not_match = []
        for wsname in self.wslist:
            wks = api.AnalysisDataService.retrieve(wsname)
            runs[wsname] = wks.getRun()

        for prop in plist:
            properties = []
            isnum = False
            for wsname in self.wslist:
                run = runs[wsname]
                if not run.hasProperty(prop):
                    message = "Workspace " + wsname + " does not have sample log " + prop
                    self.log().warning(message)
                else:
                    curprop = run.getProperty(prop)
                    if curprop.type == "string" or curprop.type == "number":
                        properties.append(curprop.value)

                    else:
                        message = (
                            "Comparison of "
                            + str(curprop.type)
                            + " properties is not yet supported. Property "
                            + prop
                            + " in the workspace "
                            + wsname
                        )
                        self.log().warning(message)

            # sometimes numbers are presented as strings
            try:
                properties = [float(val) for val in properties]
            except ValueError:
                pass
            else:
                isnum = True

            # check whether number of properties and workspaces match
            match = self.do_match(prop, properties, isnum)
            if not match:
                does_not_match.append(prop)

        return does_not_match

    def PyExec(self):
        self.wslist = self._expand_groups()

        # no sence to compare sample logs for one workspace
        if len(self.wslist) < 2:
            message = "At least 2 workspaces must be given as an input."
            self.log().error(message)
            raise RuntimeError(message)

        lognames = self.getProperty("SampleLogs").value
        self.tolerance = self.getProperty("Tolerance").value
        result = ""
        do_not_match = self.compare_properties(lognames)

        # return list of not matching properties
        if len(do_not_match) > 0:
            result = ",".join(do_not_match)

        self.setProperty("Result", result)
        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CompareSampleLogs)
