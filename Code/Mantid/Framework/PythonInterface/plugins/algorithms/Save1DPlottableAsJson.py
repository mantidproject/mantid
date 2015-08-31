#pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os

    
# See ticket #10234

class Save1DPlottableAsJson(PythonAlgorithm):
    """ Save 1D plottable data in json format from workspace. 
    """
    def category(self):
        """
        """
        return "Framework;Utility"

    def name(self):
        """
        """
        return "Save1DPlottableAsJson"

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
            "Name of the output Json file. ")
        
        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Properties
        inputwsname = self.getPropertyValue("InputWorkspace")
        outfilename = self.getPropertyValue("JsonFilename")

        # Check properties
        inputws = AnalysisDataService.retrieve(inputwsname)
        if inputws is None:
            raise ValueError(
                "Inputworkspace does not exist.")
        if inputws.axes() > 2:
            raise ValueError(
                "InputWorkspace must be one-dimensional.")
            
        if os.path.exists(outfilename):
            raise IOError(
                "Output file %s already exists" % outfilename)

        # Generate Json file
        output = self._save(inputws, outfilename)
        return

    def _save(self, inputws, outpath):
        d = self._serialize(inputws)
        import json
        json.dump(d, open(outpath, 'wt'))
        return
        
    def _serialize(self, ws):
        wname = ws.getName()
        # init dictionary
        ishist = ws.isHistogramData()
        type = "histogram" if ishist else "point"
        d = {"type": type}
        # helper
        label = lambda axis: "%s (%s)" % (
            axis.getUnit().caption(),
            axis.getUnit().symbol() or 1,
            )
        # loop over spectra
        for i in range(ws.getNumberHistograms()):
            k = "%s%s" % (wname, i)
            v = dict(
                x = list(ws.readX(i)),
                y = list(ws.readY(i)),
                e = list(ws.readE(i)),
                xlabel = label(ws.getAxis(0)),
                ylabel = label(ws.getAxis(1)),
                title = "long title of %s" % k,
                )
            d[k] = v
            continue
        return d


# Register algorithm with Mantid
AlgorithmFactory.subscribe(Save1DPlottableAsJson)
