# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
This is an example Python algorithm, showing how
to write a workspace to file in ascii format.
Note that the SaveAscii algorithm should be used instead in most cases.
"""

# This __future__ import is for Python 2/3 compatibility
from mantid.kernel import *
from mantid.api import *


class ExampleSaveAscii(PythonAlgorithm):
    def category(self):
        # defines the category the algorithm will be put in the algorithm browser
        return "Examples"

    def PyInit(self):
        # Declare properties

        # Declare a property for the output filename with a default of default_output.txt
        self.declareProperty(
            FileProperty(name="OutputFilename", defaultValue="default_output.txt", action=FileAction.Save, extensions=["txt"])
        )

        # Declare a property for the input workspace which will be written to file
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input), doc="Documentation for this property"
        )

    def PyExec(self):
        # Save the workspace to file in ascii format

        input_workspace = self.getProperty("InputWorkspace").value

        # Open the file with write permissions
        # The 'with' statement will take care of closing the file when we are done,
        # or if an error occurs
        with open(self.getPropertyValue("OutputFilename"), "w") as file_handle:
            # Get the units from the workspace to use in the file header
            x_label = input_workspace.getAxis(0).getUnit().caption()
            y_label = input_workspace.getAxis(1).getUnit().caption()

            # Write column header to file
            file_handle.write("# " + x_label + " , " + y_label + " , E\n")

            # Loop through each spectrum histogram
            for histogram_n in range(input_workspace.getNumberHistograms()):
                # Read the histogram data from the workspace
                xdata = input_workspace.readX(histogram_n)
                ydata = input_workspace.readY(histogram_n)
                edata = input_workspace.readE(histogram_n)

                # Write the spectrum histogram index to file
                file_handle.write(str(histogram_n + 1) + "\n")  # +1 to convert to 1 indexed

                # Loop through each bin
                for bin_n in range(input_workspace.blocksize()):
                    # Calculate bin center from bin boundaries
                    bin_center = xdata[bin_n] + (xdata[bin_n + 1] - xdata[bin_n]) / 2.0

                    # Write the data for the nth bin to file
                    # with a precision of 4 decimal places
                    file_handle.write("{0:.4f},{1:.4f},{2:.4f}\n".format(bin_center, ydata[bin_n], edata[bin_n]))


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExampleSaveAscii)
