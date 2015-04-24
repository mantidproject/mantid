# pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *


class PoldiDataAnalysis(PythonAlgorithm):
    """
    This workflow algorithm uses all of the POLDI specific algorithms to perform a complete data analysis,
    starting from the correlation method and preliminary 1D-fits, proceeding with either one or two passses
    of 2D-fitting.

    All resulting workspaces are grouped together at the end so that they are all in one place.
    """

    def category(self):
        return "Workflow"

    def name(self):
        return "PoldiDataAnalysis"

    def summary(self):
        return "Run all necessary steps for a complete analysis of POLDI data."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input),
            doc='MatrixWorkspace with 2D POLDI data and valid POLDI instrument.')

        self.declareProperty(
            "MaximumPeakNumber", 10, direction=Direction.Input,
            doc='Maximum number of peaks to process in the analysis.')

        self.declareProperty(
            WorkspaceProperty("ExpectedPeaks", defaultValue="", direction=Direction.Input),
            doc='TableWorkspace or WorkspaceGroup with expected peaks used for indexing.'
        )

        self.declareProperty(
            "PawleyFit", False, direction=Direction.Input,
            doc='Should the 2D-fit determine lattice parameters?'
        )

        self.declareProperty(
            "RunTwice", False, direction=Direction.Input,
            doc=('If this is activated, peaks are searched again in the residuals and the 1D- and 2D-fit is repeated '
                 'with these data.')
        )

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc='WorkspaceGroup with result data from all processing steps.'
        )

        def PyExec(self):
            pass

    AlgorithmFactory.subscribe(PoldiDataAnalysis())