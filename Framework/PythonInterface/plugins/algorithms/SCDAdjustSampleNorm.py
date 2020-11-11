# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileAction, FileProperty, IMDHistoWorkspaceProperty, PythonAlgorithm, \
    PropertyMode, MultipleFileProperty, WorkspaceProperty
from mantid.kernel import Direction, EnabledWhenProperty, PropertyCriterion, V3D, FloatArrayProperty, \
    FloatArrayLengthValidator
from mantid.simpleapi import ConvertHFIRSCDtoMDE, ConvertWANDSCDtoQ, DeleteWorkspace, DeleteWorkspaces, DivideMD, \
    Load, MergeMD, ReplicateMD, mtd
import os


class SCDAdjustSampleNorm(PythonAlgorithm):

    def category(self):
        return "Crystal\\Corrections"

    def seeAlso(self):
        return ["ConvertWANDSCDtoQ", "ConvertHFIRSCDtoMDE"]

    def name(self):
        return "SCDAdjustSampleNorm"

    def summary(self):
        return 'Takes detector scan data files or workspaces and adjusts the detector position based on detector ' \
               'height and distance if those options are given. Normalizes with detector efficiency from input ' \
               'vanadium file, and converts to Q-space.'

    def PyInit(self):
        # Input params
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=["_event.nxs", ".nxs.h5", ".nxs"],
                                                  action=FileAction.OptionalLoad),
                             doc="Input autoreduced detector scan data files to convert to Q-space.")
        self.declareProperty(
            FileProperty(name="VanadiumFile", defaultValue="", extensions=[".nxs"], direction=Direction.Input,
                         action=FileAction.OptionalLoad),
            doc="File with Vanadium normalization scan data")

        # Alternative WS inputs
        self.declareProperty("InputWorkspaces", defaultValue="", direction=Direction.Input,
                             doc="Workspace or comma-separated workspace list containing input MDHisto scan data.")
        self.declareProperty(IMDHistoWorkspaceProperty("VanadiumWorkspace", defaultValue="", direction=Direction.Input,
                                                       optional=PropertyMode.Optional),
                             doc="MDHisto workspace containing vanadium normalization data")

        # Detector adjustment options
        self.declareProperty("DetectorHeightOffset", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional distance to move detector height (relative to current position)")
        self.declareProperty("DetectorDistanceOffset", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional distance to move detector distance (relative to current position)")

        # Which conversion algorithm to use
        self.declareProperty("OutputAsMDEventWorkspace", defaultValue=True, direction=Direction.Input,
                             doc="Whether to use ConvertHFIRSCDtoQ for an MDEvent, or ConvertWANDSCDtoQ for an MDHisto")

        # MDEvent WS Specific options for ConvertHFIRSCDtoQ
        self.declareProperty(FloatArrayProperty("MinValues", [-10, -10, -10], FloatArrayLengthValidator(3),
                                                direction=Direction.Input),
                             doc="3 comma separated values, one for each q_sample dimension")
        self.declareProperty(FloatArrayProperty("MaxValues", [10, 10, 10], FloatArrayLengthValidator(3),
                                                direction=Direction.Input),
                             doc="3 comma separated values, one for each q_sample dimension; must be larger than"
                                 "those specified in MinValues")
        self.declareProperty("MergeInputs", defaultValue=False, direction=Direction.Input,
                             doc="If all inputs should be merged into one MDEvent output workspace")

        # MDHisto WS Specific options for ConvertWANDSCDtoQ
        self.declareProperty(FloatArrayProperty("BinningDim0", [-8.02, 8.02, 401], FloatArrayLengthValidator(3),
                                                direction=Direction.Input),
                             "Binning parameters for the 0th dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty(FloatArrayProperty("BinningDim1", [-0.82, 0.82, 41], FloatArrayLengthValidator(3),
                                                direction=Direction.Input),
                             "Binning parameters for the 1st dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty(FloatArrayProperty("BinningDim2", [-8.02, 8.02, 401], FloatArrayLengthValidator(3),
                                                direction=Direction.Input),
                             "Binning parameters for the 2nd dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")

        self.setPropertySettings("Filename", EnabledWhenProperty('InputWorkspaces', PropertyCriterion.IsDefault))
        self.setPropertySettings("VanadiumFile", EnabledWhenProperty('VanadiumWorkspace', PropertyCriterion.IsDefault))
        self.setPropertySettings("InputWorkspaces", EnabledWhenProperty('Filename', PropertyCriterion.IsDefault))
        self.setPropertySettings("VanadiumWorkspace", EnabledWhenProperty('VanadiumFile', PropertyCriterion.IsDefault))

        event_settings = EnabledWhenProperty('OutputAsMDEventWorkspace', PropertyCriterion.IsDefault)
        self.setPropertyGroup("MinValues", "MDEvent Settings")
        self.setPropertyGroup("MaxValues", "MDEvent Settings")
        self.setPropertyGroup("MergeInputs", "MDEvent Settings")
        self.setPropertySettings("MinValues", event_settings)
        self.setPropertySettings("MaxValues", event_settings)
        self.setPropertySettings("MergeInputs", event_settings)

        histo_settings = EnabledWhenProperty('OutputAsMDEventWorkspace', PropertyCriterion.IsNotDefault)
        self.setPropertyGroup("BinningDim0", "MDHisto Settings")
        self.setPropertyGroup("BinningDim1", "MDHisto Settings")
        self.setPropertyGroup("BinningDim2", "MDHisto Settings")
        self.setPropertySettings("BinningDim0", histo_settings)
        self.setPropertySettings("BinningDim1", histo_settings)
        self.setPropertySettings("BinningDim2", histo_settings)

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output MDWorkspace in Q-space, name is prefix if multiple input files were provided.")

    def validateInputs(self):
        issues = dict()

        filelist = self.getProperty("Filename").value
        vanfile = self.getProperty("VanadiumFile").value
        input_ws = self.getProperty("InputWorkspaces")
        van_ws = self.getProperty("VanadiumWorkspace")

        # Make sure files and workspaces aren't both set
        if len(filelist) >= 1:
            if not input_ws.isDefault:
                issues['InputWorkspaces'] = "Cannot specify both a filename and input workspace"
        else:
            if input_ws.isDefault:
                issues['Filename'] = "Either a file or input workspace must be specified"

        if len(vanfile) <= 0 and van_ws.isDefault:
            issues['VanadiumFile'] = "Either a vanadium file or vanadium workspace must be specified!"

        if len(vanfile) > 0 and not van_ws.isDefault:
            issues['VanadiumWorkspace'] = "Cannot specify both a vanadium file and workspace"

        # Verify given workspaces exist
        if not input_ws.isDefault:
            input_ws_list = input_ws.value.split(",")
            for ws in input_ws_list:
                if not mtd.doesExist(ws):
                    issues['InputWorkspaces'] = "Could not find input workspace '{}'".format(ws)

        return issues

    def PyExec(self):
        load_van = not self.getProperty("VanadiumFile").isDefault
        load_files = not self.getProperty("Filename").isDefault

        if load_files:
            datafiles = self.getProperty("Filename").value
        else:
            datafiles = self.getProperty("InputWorkspaces").value.split(",")

        vanadiumfile = self.getProperty("VanadiumFile").value
        vanws = self.getProperty("VanadiumWorkspace").value
        height = self.getProperty("DetectorHeightOffset").value
        distance = self.getProperty("DetectorDistanceOffset").value
        method = self.getProperty("OutputAsMDEventWorkspace").value

        if method:
            minvals = self.getProperty("MinValues").value
            maxvals = self.getProperty("MaxValues").value
            merge = self.getProperty("MergeInputs").value
            wslist = []
        else:
            bin0 = self.getProperty("BinningDim0").value
            bin1 = self.getProperty("BinningDim1").value
            bin2 = self.getProperty("BinningDim2").value

        out_ws = self.getPropertyValue("OutputWorkspace")
        out_ws_name = out_ws

        if load_van:
            vanws = Load(vanadiumfile, StoreInADS=False)

        has_multiple = True if len(datafiles) > 1 else False

        # Default wavelength in WANDSCDtoQ if not set in the input file
        wavelength = 1.488

        for file in datafiles:
            if load_files:
                scan = Load(file, LoadHistory=False, StoreInADS=False)
            else:
                scan = mtd[file]

            self.log().information("Processing file '{}'".format(file))

            # If processing multiple files, append the base name to the given output name
            if has_multiple:
                if load_files:
                    out_ws_name = out_ws + "_" + os.path.basename(file).strip(',.nxs')
                else:
                    out_ws_name = out_ws + "_" + file

            exp_info = scan.getExperimentInfo(0)

            # Adjust detector height and distance with new offsets
            if height != 0.0 or distance != 0.0:
                # Move all the instrument banks (MoveInstrumentComponents does not work on MDHisto Workspaces)
                component = exp_info.componentInfo()
                for bank in range(1, 4):
                    index = component.indexOfAny("bank{}".format(bank))

                    offset = V3D(0.0, height, distance)
                    pos = component.position(index)

                    offset += pos

                    component.setPosition(index, offset)

            # Get the wavelength from the file sample logs if it exists
            if exp_info.run().hasProperty("wavelength"):
                wavelength = exp_info.run().getProperty("wavelength").value

            # Use ConvertHFIRSCDtoQ (and normalize van), or use ConvertWANDSCtoQ which handles normalization itself
            if method:
                van_norm = ReplicateMD(ShapeWorkspace=scan, DataWorkspace=vanws, StoreInADS=False)
                van_norm = DivideMD(LHSWorkspace=scan, RHSWorkspace=van_norm, StoreInADS=False)
                ConvertHFIRSCDtoMDE(InputWorkspace=scan, Wavelength=wavelength, MinValues=minvals,
                                    MaxValues=maxvals, OutputWorkspace=out_ws_name)
                if merge:
                    wslist.append(out_ws_name)
            else:
                # Convert to Q space and normalize with from the vanadium
                ConvertWANDSCDtoQ(InputWorkspace=scan, NormalisationWorkspace=vanws, Frame='Q_sample',
                                  Wavelength=wavelength, NormaliseBy='Monitor', BinningDim0=bin0, BinningDim1=bin1,
                                  BinningDim2=bin2,
                                  OutputWorkspace=out_ws_name)

        if method:
            if merge and len(wslist) > 1:
                out_ws_name = out_ws
                MergeMD(InputWorkspaces=wslist, OutputWorkspace=out_ws_name)
                DeleteWorkspaces(wslist)
            DeleteWorkspace(van_norm)

        # Don't delete workspaces if they were passed in
        if load_van:
            DeleteWorkspace(vanws)
        if load_files:
            DeleteWorkspace(scan)

        self.setProperty("OutputWorkspace", out_ws_name)


AlgorithmFactory.subscribe(SCDAdjustSampleNorm)
