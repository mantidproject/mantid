# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, PropertyMode, \
    MultipleFileProperty, WorkspaceProperty, mtd
from mantid.kernel import Direction
from mantid.simpleapi import GroupWorkspaces, Load, ConvertWANDSCDtoQ, MoveInstrumentComponent
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

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output MDWorkspace in Q-space. Name is prefix if multiple input files were provided.")

    def validateInputs(self):
        issues = dict()

        # Are height and distance supposed to be set together?
        height = self.getProperty("DetectorHeightOffset").isDefault
        distance = self.getProperty("DetectorDistanceOffset").isDefault
        if not height and distance:
            issues['DetectorHeightOffset'] = "Must set the detector height offset with the detector distance offset."
        if height and not distance:
            issues['DetectorDistanceOffset'] = "Must set the detector distance with the detector height offset."

        return issues

    def PyExec(self):
        datafiles = self.getProperty("Filename").value
        vanadiumfile = self.getProperty("VanadiumFile").value
        height = self.getProperty("DetectorHeightOffset").value
        distance = self.getProperty("DetectorDistanceOffset").value
        out_ws = self.getPropertyValue("OutputWorkspace")
        out_ws_name = out_ws

        van_norm = Load(vanadiumfile)

        has_multiple = True if len(datafiles) > 1 else False

        for file in datafiles:
            scan = Load(file)

            self.log().information("Processing file '{}'".format(file))

            # If processing multiple files, append the base name to the given output name
            if has_multiple:
                out_ws_name = out_ws + "_" + os.path.basename(file)

            # Adjust detector height and distance with new offsets
            if height > 0.0:
                instrument = scan.getInstrument()

                # Move all the instrument banks
                MoveInstrumentComponent(scan, ComponentName="bank1", Y=height, Z=distance)
                MoveInstrumentComponent(scan, ComponentName="bank2", Y=height, Z=distance)
                MoveInstrumentComponent(scan, ComponentName="bank3", Y=height, Z=distance)

            # Convert to Q space and normalize with from the vanadium
            ConvertWANDSCDtoQ(InputWorkspace=scan, NormalisationWorkspace=van_norm, Frame='Q_sample',
                              NormaliseBy='Monitor', OutputWorkspace=out_ws_name)

        self.setProperty("OutputWorkspace", out_ws_name)


AlgorithmFactory.subscribe(SCDAdjustSampleNorm)
