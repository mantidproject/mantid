# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, InstrumentValidator, FileProperty, FileAction
from mantid.kernel import Direction, StringArrayProperty
import mantid.simpleapi as api


class SetDetScale(PythonAlgorithm):
    """
    Class to set instrument detScale for SaveHKL and AnvredCorrection
    """

    def category(self):
        """
        Mantid required
        """
        return "DataHandling\\Instrument"

    def name(self):
        """
        Mantid required
        """
        return "SetDetScale"

    def summary(self):
        """
        Mantid required
        """
        return "Set/change the detScale parameters for a MatrixWorkspace or PeaksWorkspace instrument"

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("Workspace", "", validator=InstrumentValidator(), direction=Direction.InOut),
            doc="MatrixWorkspace or PeaksWorkspace with instrument.",
        )

        # List of parameters
        self.declareProperty(
            StringArrayProperty("DetScaleList", direction=Direction.Input),
            doc="Comma separated list detectorNumbers:detScales eg. 13:1.046504,14:1.259293",
        )

        self.declareProperty(
            FileProperty(name="DetScaleFile", defaultValue="", action=FileAction.OptionalLoad, extensions=["txt"]),
            "Optional text file with detector number and its scale on each line separated by spaces",
        )

    def PyExec(self):
        ws = self.getProperty("Workspace").value

        # Now input all the components
        components = self.getProperty("DetScaleList").value

        listParse = []
        scaleDict = {}

        # Read scales from file
        sc_filename = self.getProperty("DetScaleFile").value
        if sc_filename:
            scfile = open(sc_filename, "r")
            lines = scfile.readlines()
            for line in lines:
                columns = line.split()  # splits on whitespace characters
                key = columns[0]
                scaleDict[key] = columns[1]

        # Overwrite any scales given in file
        for component in components:
            comp, value = component.split(":")
            key = comp
            scaleDict[key] = value

        for key in scaleDict.keys():
            listParse.append({"ParameterName": "detScale" + key, "Value": scaleDict[key]})

        for dList in listParse:
            api.SetInstrumentParameter(Workspace=ws, ParameterType="Number", **dList)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(SetDetScale)
