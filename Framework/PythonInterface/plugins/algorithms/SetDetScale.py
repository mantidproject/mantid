from mantid.api import PythonAlgorithm, AlgorithmFactory, PropertyMode, WorkspaceProperty, InstrumentValidator
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

        self.declareProperty(WorkspaceProperty("Workspace", "",
                                               validator=InstrumentValidator(),
                                               optional=PropertyMode.Optional,
                                               direction=Direction.InOut),
                             doc="MatrixWorkspace or PeaksWorkspace with instrument.")

        # List of parameters
        self.declareProperty(StringArrayProperty("DetScaleList",
                                                 direction=Direction.Input),
                             doc="Comma separated list detectorNumbers:detScales eg. 13:1.046504,14:1.259293")



    def PyExec(self):
        ws = self.getProperty("Workspace").value

        # Now input all the components
        components = self.getProperty("DetScaleList").value

        listParse = []
        for component in components:
            comp, value = component.split(":")
            listParse.append({"ParameterName":"detScale"+comp, "Value":value})

        for dList in listParse:
            api.SetInstrumentParameter(Workspace=ws,ParameterType="Number",**dList)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(SetDetScale)
