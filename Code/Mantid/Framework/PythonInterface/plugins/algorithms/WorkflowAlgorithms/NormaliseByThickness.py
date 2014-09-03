import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

class NormaliseByThickness(PythonAlgorithm):
    """
        Normalise detector counts by the sample thickness
    """

    def category(self):
        return "Workflow\\SANS"

    def name(self):
        return "NormaliseByThickness"

    def summary(self):
      return "Normalise detector counts by the sample thickness."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction = Direction.Output),
                             "Name of the workspace that will contain the normalised data")
        self.declareProperty("SampleThickness", 0.0,
                             "Optional sample thickness value. If not provided the sample-thickness run property will be used.")
        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value

        # Determine whether we should use the input thickness or try
        # to read it from the run properties
        thickness = self.getProperty("SampleThickness").value
        if thickness <= 0:
            if input_ws.getRun().hasProperty("sample-thickness"):
                thickness = input_ws.getRun().getProperty("sample-thickness").value
                if thickness <= 0:
                    Logger("NormaliseByThickness").error("NormaliseByThickness could not get the sample thickness")
                    return
            else:
                Logger("NormaliseByThickness").error("NormaliseByThickness could not get the sample thickness")
                return

        output_ws_name = self.getPropertyValue("OutputWorkspace")
        api.Scale(InputWorkspace=input_ws,
                  OutputWorkspace=output_ws_name,
                  Factor=1.0/thickness, Operation="Multiply")

        self.setProperty("OutputWorkspace", output_ws_name)
        self.setProperty("OutputMessage", "Normalised by thickness [%g cm]" % thickness)

AlgorithmFactory.subscribe(NormaliseByThickness())
