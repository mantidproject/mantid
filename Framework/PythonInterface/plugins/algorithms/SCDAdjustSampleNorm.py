# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, PropertyMode, MultipleFileProperty, \
    WorkspaceProperty, mtd
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
        return 'Takes detector scan data files, reprocesses them with an adjusted sample height if the detector \
               options are given. Normalizes with detector efficiency from input vanadium file, and converts to \
               Q-space.'

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=["_event.nxs", ".nxs.h5", ".nxs"]),
                             "Input autoreduced detector scan data files to reprocess and convert.")
        self.declareProperty(
            FileProperty(name="VanadiumFile", defaultValue="", extensions=[".nxs"], direction=Direction.Input,
                         action=FileAction.Load),
            doc="File with Vanadium normalization scan data")

        self.declareProperty("SampleHeight", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional new sample height (detector height)")
        self.declareProperty("SampleDistance", defaultValue=0.0, direction=Direction.Input,
                             doc="Optional new sample distance (detector distance)")

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output MDWorkspace in Q-space. Name is prefix if multiple input files were provided.")

    def validateInputs(self):
        issues = dict()

        # Are height and distance supposed to be set together?
        height = self.getProperty("SampleHeight").isDefault
        distance = self.getProperty("SampleDistance").isDefault
        if not height and distance:
            issues['SampleHeight'] = "Must set the sample height with the sample distance"
        if height and not distance:
            issues['SampleDistance'] = "Must set the sample distance with the sample height"

        return issues

    def PyExec(self):
        datafiles = self.getProperty("Filename").value
        vanadiumfile = self.getProperty("VanadiumFile").value
        height = self.getProperty("SampleHeight").value
        distance = self.getProperty("SampleDistance").value
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

            # Adjust sample height and reprocess data with new position
            if height > 0.0:
                exp_info = scan.getExperimentInfo(0)
                detector = exp_info.detectorInfo()
                component = exp_info.componentInfo()

                MoveInstrumentComponent(scan, ComponentName="sample-position", Y=height, Z=distance)

            # Convert to Q space and normalize with from the vanadium
            ConvertWANDSCDtoQ(InputWorkspace=scan, NormalisationWorkspace=van_norm, Frame='Q_sample',
                              NormaliseBy='Monitor', OutputWorkspace=out_ws_name)

        self.setProperty("OutputWorkspace", out_ws_name)


AlgorithmFactory.subscribe(SCDAdjustSampleNorm)
