# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

from mantid.api import *
from mantid.kernel import *

#
# The following points are recommendations for writing Python algorithms:
#  - The class name should match the file name;
#  - Each file should contain exactly one algorithm.
#


class Squares(PythonAlgorithm):
    def category(self):
        # defines the category the algorithm will be put in the algorithm browser
        return "Examples"

    def PyInit(self):
        # Integer property. IntBoundedValidator restricts values to be greater than 0
        self.declareProperty("MaxRange", -1, validator=IntBoundedValidator(lower=0), doc="A value for the end of the range(inclusive)")
        # String property. StringMandatoryValidator requires that the value not be empty
        self.declareProperty("Preamble", "", validator=StringMandatoryValidator(), doc="Required preamble")
        self.declareProperty("Sum", False, doc="If True, sum the squared values")
        self.declareProperty(FileProperty("OutputFile", "", action=FileAction.Save, extensions=["txt"]))
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), "A workspace containing the squares"
        )

    def PyExec(self):
        dummy_msg = self.getProperty("Preamble").value  # Convert to string # noqa
        endrange = self.getProperty("MaxRange").value  # Convert to int
        do_sum = self.getProperty("Sum").value  # Convert to boolean

        if endrange <= 0:
            raise RuntimeError("No values to use!")

        # Create standard 2D matrix-like workspace
        wspace = WorkspaceFactory.create("Workspace2D", NVectors=1, XLength=endrange, YLength=endrange)

        # Setup progress reporting. start/end=fractions of whole progress to report over (nreports does not have to be exact)
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=endrange + 1)  # extra call below when summing

        for i in range(1, endrange + 1):
            wspace.dataX(0)[i - 1] = 1
            wspace.dataY(0)[i - 1] = i * i
            wspace.dataE(0)[i - 1] = i
            prog_reporter.report("Setting %dth bin in workspace" % (i - 1))

        self.setProperty("OutputWorkspace", wspace)  # Stores the workspace as the given name
        if do_sum:
            summed = 0
            prog_reporter.report("Summing bin values")  # Message is left until cleared
            for i in range(1, endrange + 1):
                summed += i * i
            self.log().notice("The sum of the squares of numbers up to " + str(endrange) + " is: " + str(summed))

            filename = self.getProperty("OutputFile").value  # convert to a string
            sumfile = open(filename, "w")
            sumfile.write("The sum of the squares of numbers up to " + str(endrange) + " is: " + str(summed) + "\n")
            sumfile.close()


#############################################################################################


AlgorithmFactory.subscribe(Squares)
