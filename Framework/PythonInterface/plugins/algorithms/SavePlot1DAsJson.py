#pylint: disable=no-init,unused-variable,invalid-name,bare-except
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
        pname = plotname or workspace.getName()
        # init dictionary
        ishist = workspace.isHistogramData()
        plottype = "histogram" if ishist else "point"
        serialized = dict(
            type = plottype,
            data = dict(),
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
                ]
            serialized['data'][spectrum_no] = arr
            continue
        # axes
        # .. helper
        label = lambda axis: axis.getUnit().caption()
        def unit(axis):
            s = axis.getUnit().symbol()
            try:
                return s.latex()
            except:
                return '%s' % s
        axes = dict(
            xlabel=label(workspace.getAxis(0)),
            ylabel=label(workspace.getAxis(1)),
            xunit = unit(workspace.getAxis(0)),
            # yunit = unit(workspace.getAxis(1)),
            yunit = workspace.YUnitLabel(),
            )
        serialized['axes'] = axes
        return {pname: serialized}


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SavePlot1DAsJson)

