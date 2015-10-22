from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceGroup, MatrixWorkspace
from mantid.kernel import Direction, StringListValidator, StringArrayProperty, StringArrayLengthValidator,\
    FloatBoundedValidator
import mantid.simpleapi as api


class CompareSampleLogs(PythonAlgorithm):
    """
    Compares specified sample logs for a given list of workspaces
    """

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)

    def category(self):
        """
        Returns category
        """
        return "PythonAlgorithms;Utility\\Workspaces"

    def name(self):
        """
        Returns name
        """
        return "CompareSampleLogs"

    def summary(self):
        return "Compares specified sample logs for a given list of workspaces."

    def PyInit(self):
        validator = StringArrayLengthValidator()
        validator.setLengthMin(1)   # one group may be given
        self.declareProperty(StringArrayProperty(name="InputWorkspaces", direction=Direction.Input, validator=validator),
                             doc="Comma separated list of workspaces or groups of workspaces.")

        self.declareProperty(StringArrayProperty(name="SampleLogs", direction=Direction.Input, validator=validator),
                             doc="Comma separated list of sample logs to compare.")
        self.declareProperty("Tolerance", 1e-3, validator=FloatBoundedValidator(lower=1e-7, upper=1.0),
                             doc="Tolerance for comparison of double values.")

        actions = ['warning', 'error']
        self.declareProperty("DoNotMatchAction", "warning", StringListValidator(actions),
                             doc="Action to perform if sample logs do not match.")
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

    def compare_properties(self, lhs_run, rhs_run, plist, tolerance=5e-3):
        """
        checks whether properties match in the given runs, produces warnings
            @param lhs_run Left-hand-side run
            @param rhs_run Right-hand-side run
            @param plist   List of properties to compare
        """
        lhs_title = ""
        rhs_title = ""
        if lhs_run.hasProperty('run_title') and rhs_run.hasProperty('run_title'):
            lhs_title = lhs_run.getProperty('run_title').value
            rhs_title = rhs_run.getProperty('run_title').value

        # for TOFTOF run_titles can be identical
        if lhs_title == rhs_title:
            if lhs_run.hasProperty('run_number') and rhs_run.hasProperty('run_number'):
                lhs_title = str(lhs_run.getProperty('run_number').value)
                rhs_title = str(rhs_run.getProperty('run_number').value)

        for property_name in plist:
            if lhs_run.hasProperty(property_name) and rhs_run.hasProperty(property_name):
                lhs_property = lhs_run.getProperty(property_name)
                rhs_property = rhs_run.getProperty(property_name)
                if lhs_property.type == rhs_property.type:
                    if lhs_property.type == 'string':
                        if lhs_property.value != rhs_property.value:
                            message = "Property " + property_name + " does not match! " + \
                                lhs_title + ": " + lhs_property.value + ", but " + \
                                rhs_title + ": " + rhs_property.value
                            print message
                            self.log().warning(message)
                    elif lhs_property.type == 'number':
                        if abs(lhs_property.value - rhs_property.value) > tolerance:
                            message = "Property " + property_name + " does not match! " + \
                                lhs_title + ": " + str(lhs_property.value) + ", but " + \
                                rhs_title + ": " + str(rhs_property.value)
                            print message
                            self.log().warning(message)
                else:
                    message = "Property " + property_name + " does not match! " + \
                        lhs_title + ": " + str(lhs_property.value) + " has type " + \
                        str(lhs_property.type) + ", but " + rhs_title + ": " + \
                        str(rhs_property.value) + " has type " + str(rhs_property.type)
                    print message
                    self.log().warning(message)
            else:
                message = "Property " + property_name + " is not present in " +\
                    lhs_title + " or " + rhs_title + " - skipping comparison."
                print message
                self.log().warning(message)
        return

    def compare_mandatory(self, wslist, plist, tolerance=0.01):
        """
        Compares properties which are required to be the same.
        Produces error message and throws exception if difference is observed
        or if one of the sample logs is not found.
        Important: exits after the first difference is observed. No further check is performed.
            @param wslist  List of workspaces
            @param plist   List of properties to compare
            @param tolerance  Tolerance for comparison of the double values.
        """
        # retrieve the workspaces, form dictionary {wsname: run}
        runs = {}
        for wsname in wslist:
            wks = api.AnalysisDataService.retrieve(wsname)
            runs[wsname] = wks.getRun()

        for prop in plist:
            properties = []
            for wsname in wslist:
                run = runs[wsname]
                if not run.hasProperty(prop):
                    message = "Workspace " + wsname + " does not have sample log " + prop
                    self.log().error(message)
                    raise RuntimeError(message)

                curprop = run.getProperty(prop)
                if curprop.type == 'string':
                    properties.append(curprop.value)
                elif curprop.type == 'number':
                    properties.append(int(curprop.value/tolerance))
                else:
                    message = "Unknown type " + str(curprop.type) + " for the sample log " +\
                        prop + " in the workspace " + wsname
                    self.log().error(message)
                    raise RuntimeError(message)
            # this should never happen, but lets check
            nprop = len(properties)
            if nprop != len(wslist):
                message = "Error. Number of properties " + str(nprop) + " for property " + prop +\
                    " is not equal to number of workspaces " + str(len(wslist))
                self.log().error(message)
                raise RuntimeError(message)
            pvalue = properties[0]
            if properties.count(pvalue) != nprop:
                message = "Sample log " + prop + " is not identical in the given list of workspaces. \n" +\
                    "Workspaces: " + ", ".join(wslist) + "\n Values: " + str(properties)
                self.log().error(message)
                raise RuntimeError(message)

    def PyExec(self):
        wslist = self._expand_groups()

        # no sence to compare sample logs for one workspace
        if len(wslist) < 2:
            message = "At least 2 workspaces must be given as an input."
            self.log().error(message)
            raise RuntimeError(message)

        lognames = self.getProperty("SampleLogs").value
        tolerance = self.getProperty("Tolerance").value
        action = self.getProperty("DoNotMatchAction").value

        if action == 'error':
            self.compare_mandatory(wslist, lognames, tolerance)

        if action == 'warning':
            ws1 = api.AnalysisDataService.retrieve(wslist[0])
            run1 = ws1.getRun()
            for wsname in wslist[1:]:
                wks = api.AnalysisDataService.retrieve(wsname)
                run = wks.getRun()
                self.compare_properties(run1, run, lognames, tolerance)

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CompareSampleLogs)
