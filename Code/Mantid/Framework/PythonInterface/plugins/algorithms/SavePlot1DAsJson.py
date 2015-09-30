#pylint: disable=no-init,unused-variable
from mantid.api import *
from mantid.kernel import *


# See ticket #10234

class SavePlot1DAsJson(PythonAlgorithm):
    """ Save 1D plottable data in json format from workspace.
    """
    def category(self):
        """
        """
        return "Utility"

    def name(self):
        """
        """
        return "SavePlot1DAsJson"

    def summary(self):
        """ Return summary
        """
        return "Plottable data file in Json format"

    def require(self):
        try:
            import json
        except:
            raise ImportError("Missing json package")

    def PyInit(self):
        """ Declare properties
        """
        # this is the requirement of using this plugin
        # is there a place to register that?
        self.require()

        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),\
            "Workspace that contains plottable data")

        self.declareProperty(
            FileProperty("JsonFilename", "", FileAction.Save, ['.json']),
            "Name of the output Json file")

        self.declareProperty("PlotName", "", "Name of the output plot")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Properties
        inputwsname = self.getPropertyValue("InputWorkspace")
        outfilename = self.getPropertyValue("JsonFilename")
        plotname = self.getPropertyValue("PlotName")

        # Check properties
        inputws = AnalysisDataService.retrieve(inputwsname)
        if inputws is None:
            raise ValueError(
                "Inputworkspace does not exist.")
        if inputws.axes() > 2:
            raise ValueError(
                "InputWorkspace must be one-dimensional.")

        # Generate Json file
        self._save(inputws, outfilename, plotname)
        return

    def _save(self, inputws, outpath, plotname):
        plot = self._serialize(inputws, plotname)
        import json
        json.dump(plot, open(outpath, 'w'))
        return

    def _serialize(self, workspace, plotname):
        wname = plotname or workspace.getName()
        # init dictionary
        ishist = workspace.isHistogramData()
        plottype = "histogram" if ishist else "point"
        serialized = {"type": plottype}
        # helper
        label = lambda axis: "%s (%s)" % (
            axis.getUnit().caption(),
            axis.getUnit().symbol() or 1,
            )
        # loop over spectra
        for i in range(workspace.getNumberHistograms()):
            k = "%s%s" % (wname, i)
            value = dict(
                x=list(workspace.readX(i)),
                y=list(workspace.readY(i)),
                e=list(workspace.readE(i)),
                xlabel=label(workspace.getAxis(0)),
                ylabel=label(workspace.getAxis(1)),
                title="long title of %s" % k,
                )
            serialized[k] = value
            continue
        return serialized


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SavePlot1DAsJson)

