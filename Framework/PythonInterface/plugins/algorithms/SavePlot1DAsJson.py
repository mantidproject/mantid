# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,unused-variable,invalid-name,bare-except
from mantid.api import AlgorithmFactory, AnalysisDataService, FileAction, FileProperty, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction


# See ticket #10234


class SavePlot1DAsJson(PythonAlgorithm):
    """Save 1D plottable data in json format from workspace."""

    def category(self):
        """ """
        return "DataHandling\\Plots"

    def seeAlso(self):
        return ["SavePlot1D", "StringToPng"]

    def name(self):
        """ """
        return "SavePlot1DAsJson"

    def summary(self):
        """Return summary"""
        return "Plottable data file in Json format"

    def require(self):
        try:
            import json  # noqa
        except:
            raise ImportError("Missing json package")

    def PyInit(self):
        """Declare properties"""
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.require()

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), "Workspace that contains plottable data")

        self.declareProperty(FileProperty("JsonFilename", "", FileAction.Save, [".json"]), "Name of the output Json file")

        self.declareProperty("PlotName", "", "Name of the output plot")

        return

    def PyExec(self):
        """Main Execution Body"""
        # Properties
        inputwsname = self.getPropertyValue("InputWorkspace")
        outfilename = self.getPropertyValue("JsonFilename")
        plotname = self.getPropertyValue("PlotName")

        # Check properties
        inputws = AnalysisDataService.retrieve(inputwsname)
        if inputws is None:
            raise ValueError("Inputworkspace does not exist.")
        if inputws.axes() > 2:
            raise ValueError("InputWorkspace must be one-dimensional.")

        # Generate Json file
        self._save(inputws, outfilename, plotname)
        return

    def _save(self, inputws, outpath, plotname):
        plot = self._serialize(inputws, plotname)
        import json

        json.dump(plot, open(outpath, "w"))
        return

    def _serialize(self, workspace, plotname):
        pname = plotname or workspace.name()
        # init dictionary
        ishist = workspace.isHistogramData()
        plottype = "histogram" if ishist else "point"
        serialized = dict(
            type=plottype,
            data=dict(),
        )
        # loop over spectra
        for i in range(workspace.getNumberHistograms()):
            spectrum_no = workspace.getSpectrum(i).getSpectrumNo()
            # Why do we need label?
            # label = "%s_spectrum_%d" % (pname, spectrum_no)
            # labels.append(label)
            # or title?
            # title = "%s - spectrum %d" % (workspace.getTitle(), spectrum_no)
            arr = [
                list(workspace.readX(i)),
                list(workspace.readY(i)),
                list(workspace.readE(i)),
                list(workspace.readDx(i)),
            ]
            serialized["data"][spectrum_no] = arr
            continue
        # axes

        def unit(axis):
            s = axis.getUnit().symbol()
            try:
                return s.latex()
            except:
                return "%s" % s

        axes = dict(
            xlabel=workspace.getAxis(0).getUnit().caption(),
            ylabel=workspace.getAxis(1).getUnit().caption(),
            xunit=unit(workspace.getAxis(0)),
            # yunit = unit(workspace.getAxis(1)),
            yunit=workspace.YUnitLabel(),
        )
        serialized["axes"] = axes
        return {pname: serialized}


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SavePlot1DAsJson)
