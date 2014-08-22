from mantid.api import *
from mantid.kernel import Direction, FloatBoundedValidator
import mantid.simpleapi
import os

class SANSSubtract(PythonAlgorithm):
    """
        I(Q) subtraction
    """
    def category(self):
        """
            Return category
        """
        return "PythonAlgorithms;SANS"

    def name(self):
        """
            Return name
        """
        return "SANSSubtract"

    def summary(self):
        return "Subtract background from an I(Q) distribution."

    def PyInit(self):
        """
            Declare properties
        """
        self.declareProperty('DataDistribution', '', direction = Direction.Input,
                             doc='Name of the input workspace or file path')
        self.declareProperty('Background', '', direction = Direction.Input,
                             doc='Name of the background workspace or file path')
        self.declareProperty("ScaleFactor", 1., FloatBoundedValidator(),
                             doc="Scaling factor [Default: 1]")
        self.declareProperty("Constant", 0., FloatBoundedValidator(),
                             doc="Additive constant [Default:0]")
        self.declareProperty(FileProperty("OutputDirectory","", FileAction.OptionalDirectory),
                             doc="Directory to write the output files in [optional]")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             doc="Workspace containing data from detectors")
        return

    def _find_or_load(self, data_str):
        """
            Determine whether the input data string is the name
            of an existing workspace or is a file to be loaded.
            Return the workspace and its name.
            @param data_str: input string for the data
        """

        if AnalysisDataService.doesExist(data_str):
            data = AnalysisDataService.retrieve(data_str)
            data_ws_name = data_str
        else:
            data_ws_name = os.path.basename(data_str)
            load_alg = mantid.api.AlgorithmManager.createUnmanaged('Load')
            load_alg.setChild(True)
            load_alg.initialize()
            load_alg.setProperty("Filename", data_str)
            load_alg.setProperty("OutputWorkspace", data_ws_name)
            load_alg.execute()
            data = load_alg.getProperty("OutputWorkspace").value

        # Keep track of dQ
        dq = data.extractDx()[0]

        # Make sure we have histogram data for rebinning purposes
        op = mantid.api.AlgorithmManager.createUnmanaged("ConvertToHistogram")
        op.initialize()
        op.setChild(True)
        op.setProperty("InputWorkspace", data)
        op.setProperty("OutputWorkspace", "__histo_data_%s" % data_ws_name)
        op.execute()
        data = op.getProperty("OutputWorkspace").value

        # Make sure we have the correct units, especially important
        # if the data was loaded from an ASCII file
        data.getAxis(0).setUnit('MomentumTransfer')

        # Set the "distribution" flag on the matrix workspace
        data.setDistribution(False)

        return data, data_ws_name, dq

    def PyExec(self):
        """
            Main execution body
        """
        data_str = self.getProperty("DataDistribution").value
        background_str = self.getProperty("Background").value
        scale = self.getProperty("ScaleFactor").value
        constant = self.getProperty("Constant").value
        output_dir = self.getPropertyValue("OutputDirectory")

        # Load data or get it from the ADS
        data, data_ws_name, dq = self._find_or_load(data_str)

        # Load background or get it from the ADS
        background, back_ws_name, _ = self._find_or_load(background_str)

        # Rebin background to data workspace
        op = mantid.api.AlgorithmManager.createUnmanaged("RebinToWorkspace")
        op.initialize()
        op.setChild(True)
        op.setProperty("WorkspaceToRebin", background)
        op.setProperty("WorkspaceToMatch", data)
        op.setProperty("OutputWorkspace", "__rebinned_bck")
        op.execute()
        rebinned_bck = op.getProperty("OutputWorkspace").value

        # Output = data - scale * background + constant
        op = mantid.api.AlgorithmManager.createUnmanaged('Scale')
        op.initialize()
        op.setChild(True)
        op.setProperty("InputWorkspace", rebinned_bck)
        op.setProperty("OutputWorkspace", '__scaled_bck')
        op.setProperty("Factor", scale)
        op.setProperty("Operation", "Multiply")
        op.execute()
        scaled_bck = op.getProperty("OutputWorkspace").value

        op = mantid.api.AlgorithmManager.createUnmanaged('Minus')
        op.initialize()
        op.setChild(True)
        op.setProperty("LHSWorkspace", data)
        op.setProperty("RHSWorkspace", scaled_bck)
        op.setProperty("OutputWorkspace", '__bck_substracted')
        op.execute()
        bck_subtr = op.getProperty("OutputWorkspace").value

        op = mantid.api.AlgorithmManager.createUnmanaged('Scale')
        op.initialize()
        op.setChild(True)
        op.setProperty("InputWorkspace", bck_subtr)
        op.setProperty("OutputWorkspace", '__corrected_output')
        op.setProperty("Factor", constant)
        op.setProperty("Operation", "Add")
        op.execute()
        output = op.getProperty("OutputWorkspace").value

        # Put back dQ
        dq_scaled = output.dataDx(0)
        for i in range(len(dq)):
            dq_scaled[i] = dq[i]

        self.setProperty("OutputWorkspace", output)

        # Save the output to disk as needed
        if len(output_dir)>0:
            root_name, ext = os.path.splitext(data_ws_name)
            op = mantid.api.AlgorithmManager.createUnmanaged('SaveCanSAS1D')
            op.initialize()
            op.setChild(True)
            op.setProperty("InputWorkspace", output)
            op.setProperty("Filename", os.path.join(output_dir, root_name+'_corr.xml'))
            op.setProperty("RadiationSource", "Spallation Neutron Source")
            op.execute()

            op = mantid.api.AlgorithmManager.createUnmanaged("SaveAscii")
            op.initialize()
            op.setChild(True)
            op.setProperty("Filename", os.path.join(output_dir, root_name+'_corr.txt'))
            op.setProperty("InputWorkspace", output)
            op.setProperty("Separator", "Tab")
            op.setProperty("CommentIndicator", "# ")
            op.setProperty("WriteXError", True)
            op.setProperty("WriteSpectrumID", False)
            op.execute()

        return

AlgorithmFactory.subscribe(SANSSubtract)
