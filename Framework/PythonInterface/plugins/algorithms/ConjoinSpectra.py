# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import mtd, AlgorithmFactory, PythonAlgorithm, TextAxis, WorkspaceGroup, WorkspaceProperty
from mantid.kernel import logger, Direction, StringListValidator, StringMandatoryValidator
from mantid.simpleapi import ConjoinWorkspaces, DeleteWorkspace, ExtractSingleSpectrum, RenameWorkspace


class ConjoinSpectra(PythonAlgorithm):
    """
    Conjoins spectra from several workspaces into a single workspace

    Spectra to be conjoined must be equally binned in order for ConjoinSpectra
    to work. If necessary use RebinToWorkspace first.
    """

    def category(self):
        return "Transforms\\Merging"

    def seeAlso(self):
        return ["AppendSpectra", "ConjoinWorkspaces"]

    def name(self):
        return "ConjoinSpectra"

    def summmary(self):
        return "Joins individual spectra from a range of workspaces into a single workspace for plotting or further analysis."

    def PyInit(self):
        self.declareProperty(
            "InputWorkspaces",
            "",
            validator=StringMandatoryValidator(),
            doc="Comma seperated list of workspaces to use, group workspaces " + "will automatically include all members.",
        )
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Name the workspace that will contain the result"
        )
        self.declareProperty("WorkspaceIndex", 0, doc="The workspace index of the spectra in each workspace to extract. Default: 0")
        self.declareProperty(
            "LabelUsing", "", doc="The name of a log value used to label the resulting spectra. " + "Default: The source workspace name"
        )
        labelValueOptions = ["Mean", "Median", "Maximum", "Minimum", "First Value"]
        self.declareProperty(
            "LabelValue",
            "Mean",
            validator=StringListValidator(labelValueOptions),
            doc="How to derive the value from a time series property",
        )

    def PyExec(self):
        # get parameter values
        wsOutput = self.getPropertyValue("OutputWorkspace")
        wsIndex = int(self.getPropertyValue("WorkspaceIndex"))
        wsString = self.getPropertyValue("InputWorkspaces").strip()
        labelUsing = self.getPropertyValue("LabelUsing").strip()
        labelValue = self.getPropertyValue("LabelValue")

        # internal values
        wsTemp = "__ConjoinSpectra_temp"
        loopIndex = 0

        # get the wokspace list
        wsNames = []
        for wsName in wsString.split(","):
            # check the ws is in mantid
            ws = mtd[wsName.strip()]
            # if we cannot find the ws then stop
            if ws is None:
                raise RuntimeError("Cannot find workspace '" + wsName.strip() + "', aborting")
            if isinstance(ws, WorkspaceGroup):
                wsNames.extend(ws.getNames())
            else:
                wsNames.append(wsName)

        ta = TextAxis.create(len(wsNames))
        if mtd.doesExist(wsOutput):
            DeleteWorkspace(Workspace=wsOutput)
        for wsName in wsNames:
            # extract the spectrum
            ExtractSingleSpectrum(InputWorkspace=wsName, OutputWorkspace=wsTemp, WorkspaceIndex=wsIndex)

            labelString = ""
            if labelUsing != "":
                labelString = self.GetLogValue(mtd[wsName.strip()], labelUsing, labelValue)
            if labelString == "":
                labelString = wsName + "_" + str(wsIndex)
            ta.setLabel(loopIndex, labelString)
            loopIndex += 1
            if mtd.doesExist(wsOutput):
                ConjoinWorkspaces(InputWorkspace1=wsOutput, InputWorkspace2=wsTemp, CheckOverlapping=False)
                if mtd.doesExist(wsTemp):
                    DeleteWorkspace(Workspace=wsTemp)
            else:
                RenameWorkspace(InputWorkspace=wsTemp, OutputWorkspace=wsOutput)

        wsOut = mtd[wsOutput]
        # replace the spectrun axis
        wsOut.replaceAxis(1, ta)

        self.setProperty("OutputWorkspace", wsOut)

    def GetLogValue(self, ws, labelUsing, labelValue):
        labelString = ""
        run = ws.getRun()
        try:
            prop = run.getProperty(labelUsing)
            try:
                stats = prop.getStatistics()
                if labelValue == "Mean":
                    labelString = str(stats.mean)
                elif labelValue == "Median":
                    labelString = str(stats.median)
                elif labelValue == "Maximum":
                    labelString = str(stats.maximum)
                elif labelValue == "Minimum":
                    labelString = str(stats.minimum)
                else:
                    labelString = str(prop.value[0])
            except AttributeError:
                # this is not a time series property - just return the value
                labelString = str(prop.value)
        except RuntimeError:
            # failed to find the property
            # log and pass out an empty string
            logger.information("Could not find log " + labelUsing + " in workspace " + str(ws) + " using workspace label instead.")
        return labelString


AlgorithmFactory.subscribe(ConjoinSpectra)
