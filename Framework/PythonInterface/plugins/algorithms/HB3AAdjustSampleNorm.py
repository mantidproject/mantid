# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (AlgorithmFactory, FileAction, FileProperty,
                        IMDHistoWorkspace, IMDHistoWorkspaceProperty, PythonAlgorithm,
                        Progress, PropertyMode, MultipleFileProperty, WorkspaceProperty)
from mantid.kernel import (Direction, EnabledWhenProperty,
                           PropertyCriterion, FloatArrayProperty, FloatArrayLengthValidator,
                           FloatPropertyWithValue, V3D, StringListValidator)
from mantid.simpleapi import (ConvertHFIRSCDtoMDE, ConvertWANDSCDtoQ, CloneMDWorkspace,
                              DeleteWorkspace, DeleteWorkspaces, DivideMD, LoadMD, MergeMD,
                              ReplicateMD, SetGoniometer, mtd, GroupWorkspaces, RenameWorkspace)
import os
import numpy as np


class HB3AAdjustSampleNorm(PythonAlgorithm):

    def category(self):
        return "Crystal\\Corrections"

    def seeAlso(self):
        return ["ConvertWANDSCDtoQ", "ConvertHFIRSCDtoMDE", "HB3AFindPeaks", "HB3APredictPeaks", "HB3AIntegratePeaks"]

    def name(self):
        return "HB3AAdjustSampleNorm"

    def summary(self):
        return 'Adjusts the detector position based on a detector height and distance offset and normalizes with ' \
               'detector efficiency from a vanadium file or workspace, and converts the input to Q-space.'

    def PyInit(self):
        # Input params
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=[".nxs.h5", ".nxs"],
                                                  action=FileAction.OptionalLoad),
                             doc="Input autoreduced detector scan data files to convert to Q-space.")
        self.declareProperty(
            FileProperty(name="VanadiumFile", defaultValue="", extensions=[".nxs"], direction=Direction.Input,
                         action=FileAction.OptionalLoad),
            doc="File with Vanadium normalization scan data")

        self.declareProperty('NormaliseBy', 'Time', StringListValidator(['None', 'Time', 'Monitor']),
                             "Normalise to monitor, time or None.")

        # Alternative WS inputs
        self.declareProperty("InputWorkspaces", defaultValue="", direction=Direction.Input,
                             doc="Workspace or comma-separated workspace list containing input MDHisto scan data.")
        self.declareProperty(IMDHistoWorkspaceProperty("VanadiumWorkspace", defaultValue="", direction=Direction.Input,
                                                       optional=PropertyMode.Optional),
                             doc="MDHisto workspace containing vanadium normalization data")

        # Detector adjustment options
        self.declareProperty("DetectorHeightOffset", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional distance (in meters) to move detector height (relative to current position)")
        self.declareProperty("DetectorDistanceOffset", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional distance (in meters) to move detector distance (relative to current position)")

        self.declareProperty(FloatPropertyWithValue("Wavelength", # EMPTY_DBL so it shows as blank in GUI
                                                    FloatPropertyWithValue.EMPTY_DBL),
                             doc="Optional wavelength value to use as backup if one was not found in the sample log")

        # Which conversion algorithm to use
        self.declareProperty("OutputType", "Q-sample events", StringListValidator(['Q-sample events', 'Q-sample histogram', 'Detector']),
                             direction=Direction.Input,
                             doc="Whether to use ConvertHFIRSCDtoQ for an MDEvent, or ConvertWANDSCDtoQ for an MDHisto")

        self.declareProperty("ScaleByMotorStep", False,
                             "If True then the intensity of the output in Q space will be scaled by the motor step size. "
                             "This will allow directly comparing the intensity of data measure with diffrent motor step sizes.")

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
        self.declareProperty(FloatArrayProperty("BinningDim1", [-2.52, 2.52, 126], FloatArrayLengthValidator(3),
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

        self.setPropertySettings("ScaleByMotorStep", EnabledWhenProperty('OutputType', PropertyCriterion.IsNotEqualTo, "Detector"))

        event_settings = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'Q-sample events')
        self.setPropertyGroup("MinValues", "MDEvent Settings")
        self.setPropertyGroup("MaxValues", "MDEvent Settings")
        self.setPropertyGroup("MergeInputs", "MDEvent Settings")
        self.setPropertySettings("MinValues", event_settings)
        self.setPropertySettings("MaxValues", event_settings)
        self.setPropertySettings("MergeInputs", event_settings)

        histo_settings = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'Q-sample histogram')
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
        wavelength = self.getProperty("Wavelength")

        # Make sure files and workspaces aren't both set
        if len(filelist) >= 1:
            if not input_ws.isDefault:
                issues['InputWorkspaces'] = "Cannot specify both a filename and input workspace"
        else:
            if input_ws.isDefault:
                issues['Filename'] = "Either a file or input workspace must be specified"

        if len(vanfile) > 0 and not van_ws.isDefault:
            issues['VanadiumWorkspace'] = "Cannot specify both a vanadium file and workspace"

        # Verify given workspaces exist
        if not input_ws.isDefault:
            input_ws_list = list(map(str.strip, input_ws.value.split(",")))
            for ws in input_ws_list:
                if not mtd.doesExist(ws):
                    issues['InputWorkspaces'] = "Could not find input workspace '{}'".format(ws)
                else:
                    # If it does exist, make sure the workspace is an MDHisto with 3 dimensions
                    if not isinstance(mtd[ws], IMDHistoWorkspace):
                        issues['InputWorkspaces'] = "Workspace '{}' must be a MDHistoWorkspace".format(ws)
                    elif mtd[ws].getNumDims() != 3:
                        issues['InputWorkspaces'] = "Workspace '{}' expected to have 3 dimensions".format(ws)

        if not wavelength.isDefault:
            if wavelength.value <= 0.0:
                issues['Wavelength'] = "Wavelength should be greater than zero"

        return issues

    def PyExec(self):
        load_van = not self.getProperty("VanadiumFile").isDefault
        load_files = not self.getProperty("Filename").isDefault

        output = self.getProperty("OutputType").value

        if load_files:
            datafiles = self.getProperty("Filename").value
        else:
            datafiles = list(map(str.strip, self.getProperty("InputWorkspaces").value.split(",")))

        prog = Progress(self, 0.0, 1.0, len(datafiles) + 1)

        vanadiumfile = self.getProperty("VanadiumFile").value
        vanws = self.getProperty("VanadiumWorkspace").value
        height = self.getProperty("DetectorHeightOffset").value
        distance = self.getProperty("DetectorDistanceOffset").value

        wslist = []

        out_ws = self.getPropertyValue("OutputWorkspace")
        out_ws_name = out_ws

        if load_van:
            vanws = LoadMD(vanadiumfile, StoreInADS=False)

        has_multiple = len(datafiles) > 1

        for in_file in datafiles:
            if load_files:
                scan = LoadMD(in_file, LoadHistory=False, OutputWorkspace="__scan")
            else:
                scan = mtd[in_file]

            # Make sure the workspace has experiment info, otherwise SetGoniometer will add some, causing issues.
            if scan.getNumExperimentInfo() == 0:
                raise RuntimeError("No experiment info was found in '{}'".format(in_file))

            prog.report()
            self.log().information("Processing '{}'".format(in_file))

            SetGoniometer(Workspace=scan, Axis0='omega,0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2='phi,0,1,0,-1', Average=False)
            # If processing multiple files, append the base name to the given output name
            if has_multiple:
                if load_files:
                    out_ws_name = out_ws + "_" + os.path.basename(in_file).strip(',.nxs')
                else:
                    out_ws_name = out_ws + "_" + in_file
                wslist.append(out_ws_name)

            exp_info = scan.getExperimentInfo(0)
            self.__move_components(exp_info, height, distance)

            # Get the wavelength from experiment info if it exists, or fallback on property value
            wavelength = self.__get_wavelength(exp_info)

            # set the run number to be the same as scan number, this will be used for peaks
            if not exp_info.run().hasProperty('run_number') and exp_info.run().hasProperty('scan'):
                try:
                    exp_info.mutableRun().addProperty('run_number', int(exp_info.run().getProperty('scan').value), True)
                except ValueError:
                    # scan must be a int
                    pass

            # Use ConvertHFIRSCDtoQ (and normalize van), or use ConvertWANDSCtoQ which handles normalization itself
            if output == "Q-sample events":
                norm_data = self.__normalization(scan, vanws, load_files)
                minvals = self.getProperty("MinValues").value
                maxvals = self.getProperty("MaxValues").value
                merge = self.getProperty("MergeInputs").value
                ConvertHFIRSCDtoMDE(InputWorkspace=norm_data, Wavelength=wavelength, MinValues=minvals,
                                    MaxValues=maxvals, OutputWorkspace=out_ws_name)
                DeleteWorkspace(norm_data)
            elif output == 'Q-sample histogram':
                bin0 = self.getProperty("BinningDim0").value
                bin1 = self.getProperty("BinningDim1").value
                bin2 = self.getProperty("BinningDim2").value
                # Convert to Q space and normalize with from the vanadium
                ConvertWANDSCDtoQ(InputWorkspace=scan, NormalisationWorkspace=vanws, Frame='Q_sample',
                                  Wavelength=wavelength, NormaliseBy=self.getProperty("NormaliseBy").value,
                                  BinningDim0=bin0, BinningDim1=bin1, BinningDim2=bin2,
                                  OutputWorkspace=out_ws_name)
                if load_files:
                    DeleteWorkspace(scan)
            else:
                norm_data = self.__normalization(scan, vanws, load_files)
                RenameWorkspace(norm_data, OutputWorkspace=out_ws_name)

        if has_multiple:
            out_ws_name = out_ws
            if output == "Q-sample events" and merge:
                MergeMD(InputWorkspaces=wslist, OutputWorkspace=out_ws_name)
                DeleteWorkspaces(wslist)
            else:
                GroupWorkspaces(InputWorkspaces=wslist, OutputWorkspace=out_ws_name)

        # Don't delete workspaces if they were passed in
        if load_van:
            DeleteWorkspace(vanws)

        self.setProperty("OutputWorkspace", out_ws_name)

    def __normalization(self, data, vanadium, load_files):
        if vanadium:
            norm_data = ReplicateMD(ShapeWorkspace=data, DataWorkspace=vanadium)
            norm_data = DivideMD(LHSWorkspace=data, RHSWorkspace=norm_data)
        elif load_files:
            norm_data = data
        else:
            norm_data = CloneMDWorkspace(data)

        if self.getProperty("ScaleByMotorStep").value and self.getProperty("OutputType").value != "Detector":
            run = data.getExperimentInfo(0).run()
            scan_log = 'omega' if np.isclose(run.getTimeAveragedStd('phi'), 0.0) else 'phi'
            scan_axis = run[scan_log].value
            scan_step = (scan_axis[-1]-scan_axis[0])/(scan_axis.size-1)
            norm_data *= scan_step

        normaliseBy = self.getProperty("NormaliseBy").value

        monitors = np.asarray(data.getExperimentInfo(0).run().getProperty('monitor').value)
        times = np.asarray(data.getExperimentInfo(0).run().getProperty('time').value)

        if load_files and vanadium:
            DeleteWorkspace(data)

        if normaliseBy == "Monitor":
            scale = monitors
        elif normaliseBy == "Time":
            scale = times
        else:
            return norm_data

        if vanadium:
            if normaliseBy == "Monitor":
                scale /= vanadium.getExperimentInfo(0).run().getProperty('monitor').value[0]
            elif normaliseBy == "Time":
                scale /= vanadium.getExperimentInfo(0).run().getProperty('time').value[0]

        norm_data.setSignalArray(norm_data.getSignalArray()/scale)
        norm_data.setErrorSquaredArray(norm_data.getErrorSquaredArray()/scale**2)

        return norm_data

    def __move_components(self, exp_info, height, distance):
        """
        Moves all instrument banks by a given height (y) and distance (x-z) in meters,
        relative to the current instrument position.
        :param exp_info: experiment info of the run, used to get the instrument components
        :param height: Distance to move the instrument along y axis
        :param distance: Distance to move the instrument in the x-z plane
        """
        # Adjust detector height and distance with new offsets
        component = exp_info.componentInfo()
        for bank in range(1, 4):
            # Set height offset (y) first on bank?
            index = component.indexOfAny("bank{}".format(bank))

            if height != 0.0:
                offset = V3D(0, height, 0)
                pos = component.position(index)
                offset += pos
                component.setPosition(index, offset)

            # Set distance offset to detector (x,z) on bank?/panel
            if distance != 0.0:
                panel_index = int(component.children(index)[0])  # should only have one child
                panel_pos = component.position(panel_index)
                panel_rel_pos = component.relativePosition(panel_index)
                # need to move detector in direction in x-z plane
                panel_offset = panel_rel_pos * (distance / panel_rel_pos.norm())
                panel_offset += panel_pos
                component.setPosition(panel_index, panel_offset)

    def __get_wavelength(self, exp_info):
        """
        Gets the wavelength from experiment info if provided, otherwise it will try to get the value
        from the algorithm property. Throws a RuntimeError if a wavelength cannot be found from either.
        :param exp_info: The experiment info of the run to lookup set wavelength value
        :return: wavelength value from experiment info if set, or from wavelength property
        """
        if exp_info.run().hasProperty("wavelength"):
            return exp_info.run().getProperty("wavelength").value
        else:
            # Set wavelength value from the backup property, if provided
            wl_prop = self.getProperty("Wavelength")
            if not wl_prop.isDefault:
                return wl_prop.value
            else:
                # If wavelength value not set, throw an error
                raise RuntimeError("Wavelength not found in sample log and was not provided as input to the algorithm")


AlgorithmFactory.subscribe(HB3AAdjustSampleNorm)
