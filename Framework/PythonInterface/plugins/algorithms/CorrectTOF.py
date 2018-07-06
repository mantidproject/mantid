from __future__ import (absolute_import, division, print_function)

import numpy as np
import scipy as sp
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceUnitValidator, \
                       InstrumentValidator, ITableWorkspaceProperty
from mantid.kernel import Direction, CompositeValidator
# pylint: disable=no-name-in-module
from mantid.simpleapi import CloneWorkspace, MaskDetectors


class CorrectTOF (PythonAlgorithm):
    """ Apply time-of-flight correction
    """

    def __init__(self):
        PythonAlgorithm.__init__(self)

    def category(self):
        """ Return category
        """
        return "Workflow\\MLZ\\TOFTOF;Transforms\\Axes"

    def seeAlso(self):
        return [ "TOFTOFMergeRuns","TOFTOFCropWorkspace" ]

    def name(self):
        """ Return name
        """
        return "CorrectTOF"

    def summary(self):
        return "Applies correction to TOF using the fitted elastic peak position."

    def PyInit(self):
        """ Declare properties
        """
        validator = CompositeValidator()
        validator.add(WorkspaceUnitValidator("TOF"))
        validator.add(InstrumentValidator())
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input, validator=validator),
                             doc="Input workspace.")
        self.declareProperty(ITableWorkspaceProperty("EPPTable", "", direction=Direction.Input),
                             doc="Input EPP table. May be produced by FindEPP algorithm.")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="Name of the workspace that will contain the results")
        return

    def validateInputs(self):
        issues = dict()
        input_workspace = self.getProperty("InputWorkspace").value

        # check for required properties
        run = input_workspace.getRun()
        if not run.hasProperty('wavelength'):
            issues['InputWorkspace'] = "Input workpsace must have sample log wavelength."
        if not run.hasProperty('TOF1'):
            issues['InputWorkspace'] = "Input workpsace must have sample log TOF1."
        # check EPP table
        table = self.getProperty("EPPTable").value
        if table.rowCount() != input_workspace.getNumberHistograms():
            issues['EPPTable'] = "Number of rows in the table must match to the input workspace dimension."
        # table must have 'PeakCentre' column
        if 'PeakCentre' not in table.getColumnNames():
            issues['EPPTable'] = "EPP Table must have the PeakCentre column."

        return issues

    def PyExec(self):
        """ Main execution body
        """
        inputws = self.getProperty("InputWorkspace").value
        epptable = self.getProperty("EPPTable").value
        outws_name = self.getPropertyValue("OutputWorkspace")

        run = inputws.getRun()
        tof1 = float(run.getLogData('TOF1').value)
        wavelength = float(run.getLogData('wavelength').value)
        velocity = sp.constants.h/(sp.constants.m_n*wavelength*1e-10)   # m/s
        instrument = inputws.getInstrument()
        sample = instrument.getSample()
        t_fit = np.array(epptable.column('PeakCentre') ) - tof1
        outws = CloneWorkspace(inputws, OutputWorkspace=outws_name)
        # mask detectors with EPP=0
        bad_data = np.where(t_fit <= 0)[0]
        if len(bad_data) > 0:
            self.log().warning("Detectors " + str(bad_data) + " have EPP=0 and will be masked.")
            MaskDetectors(outws, DetectorList=bad_data)

        for idx in range(outws.getNumberHistograms()):
            # correct TOF only if fit of EPP was succesful
            if t_fit[idx] > 0:
                det = instrument.getDetector(outws.getSpectrum(idx).getDetectorIDs()[0])
                sdd = det.getDistance(sample)
                t2_el = sdd*1.0e+6/velocity         # microseconds
                newX = inputws.readX(idx) + t2_el - t_fit[idx]
                outws.setX(idx, newX)

        self.setProperty("OutputWorkspace", outws)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(CorrectTOF)
