# pylint: disable=no-name-in-module
from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, ITableWorkspaceProperty
from mantid.kernel import Direction
from mantid.simpleapi import Fit, CreateEmptyTableWorkspace
import numpy as np

class FindEPP(PythonAlgorithm):
    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.workspace = None

    def category(self):
        return "Workflow\\MLZ\\TOFTOF;Utility"

    def name(self):
        return "FindEPP"

    def summary(self):
        return "Performs Gaussian fit of all spectra to find the elastic peak position."

    def PyInit(self):
        # input
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
                             doc="Input Sample or Vanadium workspace")

        # output
        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             doc="The name of the table workspace that will be created.")

    def do_fit_gaussian(self, index):
        """
        Performs gaussian fit of the specified spectrum
        returns fitStatus, chiSq, covarianceTable, paramTable
        """
        result = None

        x_values = np.array(self.workspace.readX(index))
        y_values = np.array(self.workspace.readY(index))

        # get peak centre position, assuming that it is the point with the highest value
        imax   = np.argmax(y_values)
        height = y_values[imax]

        # check for zero or negative signal
        if height <= 0.:
            self.log().warning("Workspace %s, detector %d has maximum <= 0" % (self.workspace.getName(), index))
            return result

        # Get the positions around the max, where the y value first drops down 0.5*height
        # This is a better approach when there is a higher noise in the tails in real data
        y_right = y_values[imax+1:]
        y_left = y_values[0:imax]
        y_left = y_left[::-1]
        fwhm_right = np.argmax(y_right < 0.5 * height)
        fwhm_left = np.argmax(y_left < 0.5 * height)
        fwhm_pos = np.minimum(imax+fwhm_right,x_values.size-1)
        fwhm_neg = np.maximum(imax-fwhm_left,0)

        self.log().debug("(spectrum %d) : FWHM lower edge is %d" % (index,fwhm_neg))
        self.log().debug("(spectrum %d) : FWHM upper edge is %d" % (index,fwhm_pos))

        if fwhm_pos - fwhm_neg + 1 < 3:
            self.log().warning("Spectrum " + str(index) + " in workspace " + self.workspace.getName() +
                               " has a too narrow peak. Cannot guess sigma. Check your data.")
            return result

        fwhm = x_values[fwhm_pos] - x_values[fwhm_neg]

        self.log().debug("(spectrum %d) : FWHM is %f" % (index, fwhm))

        sigma = fwhm / (2.*np.sqrt(2.*np.log(2.)))

        # execute Fit algorithm
        tryCentre = x_values[imax]
        fitFun = "name=Gaussian,PeakCentre=%s,Height=%s,Sigma=%s" % (tryCentre,height,sigma)

        startX = tryCentre - 3.0*fwhm
        endX   = tryCentre + 3.0*fwhm

        # pylint: disable=assignment-from-none
        # result = fitStatus, chiSq, covarianceTable, paramTable
        result = Fit(InputWorkspace=self.workspace, WorkspaceIndex=index, StartX = startX, EndX=endX,
                     Output='EPPfit', Function=fitFun, CreateOutput=True, OutputParametersOnly=True)

        return result

    def PyExec(self):
        self.workspace = self.getProperty("InputWorkspace").value
        outws_name = self.getPropertyValue("OutputWorkspace")

        # create table and columns
        outws = CreateEmptyTableWorkspace(OutputWorkspace=outws_name)
        columns = ["PeakCentre", "PeakCentreError", "Sigma", "SigmaError", "Height", "HeightError", "chiSq"]
        nextrow = dict.fromkeys(["WorkspaceIndex"] + columns + ["FitStatus"])
        outws.addColumn(type="int", name="WorkspaceIndex", plottype=1)  # x
        for col in columns:
            outws.addColumn(type="double", name=col)
        outws.addColumn(type="str", name="FitStatus")

        nb_hist = self.workspace.getNumberHistograms()
        for idx in range(nb_hist):
            nextrow["WorkspaceIndex"] = idx
            result = self.do_fit_gaussian(idx)
            if not result:
                for col in columns:
                    nextrow[col] = 0
                nextrow["FitStatus"] = "failed"
            else:
                nextrow["FitStatus"] = result[0]
                nextrow["chiSq"] = result[1]
                ptable = result[3]
                for num in range(ptable.rowCount() - 1):
                    row = ptable.row(num)
                    name = row["Name"]
                    nextrow[name] = row["Value"]
                    nextrow[name+"Error"] = row["Error"]

            # self.log().debug("Next row= " + str(nextrow))
            outws.addRow(nextrow)

        self.setProperty("OutputWorkspace", outws)
        return

AlgorithmFactory.subscribe(FindEPP)
