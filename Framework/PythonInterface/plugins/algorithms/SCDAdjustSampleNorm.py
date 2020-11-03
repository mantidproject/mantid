# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, PropertyMode, \
    MultipleFileProperty, WorkspaceProperty
from mantid.kernel import Direction, V3D, FloatArrayProperty, FloatArrayLengthValidator
from mantid.simpleapi import DeleteWorkspace, Load, ConvertWANDSCDtoQ
import os


class SCDAdjustSampleNorm(PythonAlgorithm):

    def category(self):
        return "Crystal\\Corrections"

    def seeAlso(self):
        return ["ConvertWANDSCDtoQ", "SingleCrystalDiffuseReduction"]

    def name(self):
        return "SCDAdjustSampleNorm"

    def summary(self):
        return 'Takes detector scan data files, adjusts the detector position based on detector height and distance ' \
               'if the options are given. Normalizes with detector efficiency from input vanadium file, ' \
               'and converts to Q-space. '

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=["_event.nxs", ".nxs.h5", ".nxs"]),
                             "Input autoreduced detector scan data files to convert to Q-space.")
        self.declareProperty(
            FileProperty(name="VanadiumFile", defaultValue="", extensions=[".nxs"], direction=Direction.Input,
                         action=FileAction.Load),
            doc="File with Vanadium normalization scan data")

        self.declareProperty("DetectorHeightOffset", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional distance to move detector height (relative to current position)")
        self.declareProperty("DetectorDistanceOffset", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional distance to move detector distance (relative to current position)")

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

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output MDWorkspace in Q-space, name is prefix if multiple input files were provided.")

    def PyExec(self):
        datafiles = self.getProperty("Filename").value
        vanadiumfile = self.getProperty("VanadiumFile").value
        height = self.getProperty("DetectorHeightOffset").value
        distance = self.getProperty("DetectorDistanceOffset").value
        bin0 = self.getProperty("BinningDim0").value
        bin1 = self.getProperty("BinningDim1").value
        bin2 = self.getProperty("BinningDim2").value
        out_ws = self.getPropertyValue("OutputWorkspace")
        out_ws_name = out_ws

        van_norm = Load(vanadiumfile)

        has_multiple = True if len(datafiles) > 1 else False

        # Default wavelength in WANDSCDtoQ if not set in the input file
        wavelength = 1.488

        for file in datafiles:
            scan = Load(file, LoadHistory=False)

            self.log().information("Processing file '{}'".format(file))

            # If processing multiple files, append the base name to the given output name
            if has_multiple:
                out_ws_name = out_ws + "_" + os.path.basename(file).strip(',.nxs')

            exp_info = scan.getExperimentInfo(0)

            # Adjust detector height and distance with new offsets
            if height != 0.0 or distance != 0.0:
                # Move all the instrument banks
                component = exp_info.componentInfo()
                for bank in range(1, 4):
                    index = component.indexOfAny("bank{}".format(bank))

                    offset = V3D(0.0, height, distance)
                    pos = component.position(index)

                    offset += pos

                    component.setPosition(index, offset)

            # Get the wavelength from the file if it exists
            if (exp_info.run().hasProperty("wavelength")):
                wavelength = exp_info.run().getProperty("wavelength").value

            # Convert to Q space and normalize with from the vanadium
            ConvertWANDSCDtoQ(InputWorkspace=scan, NormalisationWorkspace=van_norm, Frame='Q_sample',
                              Wavelength=wavelength, NormaliseBy='Monitor', BinningDim0=bin0, BinningDim1=bin1,
                              BinningDim2=bin2,
                              OutputWorkspace=out_ws_name)

        DeleteWorkspace(van_norm)
        DeleteWorkspace(scan)

        self.setProperty("OutputWorkspace", out_ws_name)


AlgorithmFactory.subscribe(SCDAdjustSampleNorm)
