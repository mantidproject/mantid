#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import FloatArrayProperty, Direction
import numpy as np


class GetQsInQENSData(PythonAlgorithm):
    """Extract Q-values from a MatrixWorkspace containing QENS data
    """

    def category(self):
        return "Inelastic\\Indirect"

    def name(self):
        return GetQsInQENSData

    def summary(self):
        return "Get Q-values in the vertical axis of a MatrixWorkspace containing QENS S(Q,E) of S(theta,E) data"

    def PyInit(self):
        """ Algorithm properties
        """
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             doc="Input QENS data as MatrixWorkspace")
        self.declareProperty("RaiseMode", False,
                             doc="Set to True if an Exception, instead of an empty list of Q values, is desired.")
        self.declareProperty(FloatArrayProperty("Qvalues", Direction.Output))

    def validateInputs(self):
        """Shallow check that input workspace is kosher
        Returns:
            @return dictionary of messages describing the issues
        """
        issues = {}
        if self.getProperty("InputWorkspace").value is None:
            issues["InputWorkspace"] = "InputWorkspace is not a MatrixWorkspace"
        return issues

    def _extract_qvalues(self, workspace):
        """Attempt to extract or compute the Q values
        Args:
            @param workspace:

        Returns:
            @return python list of Q values

        Except:
            @except unable to extract the list of Q values
        """
        # Check if the vertical axis has units of Momentum transfer
        number_spectra = workspace.getNumberHistograms()
        try:
            axis = workspace.getAxis(1)
        except:
            raise RuntimeError("Emtpy vertical axis")
        if axis.getUnit().unitID() == "MomentumTransfer":
            try:
                qvalues = axis.extractValues()
            except:
                raise RuntimeError("Unable to extract the Q values")
            # convert to point values if qvalues is histogram data
            if len(qvalues) == number_spectra+1:
                qvalues = (qvalues[1:]+qvalues[:-1])/2
            qvalues = qvalues.tolist()
        else:
            # Attempt to compute the Q values
            qvalues=list()
            try:
                for index in range(number_spectra):
                    detector = workspace.getDetector(index)
                    efixed = workspace.getEFixed(detector.getID())  # in meV
                    wavelength=9.044567/np.sqrt(efixed)  # in Angstroms
                    usignTheta = 0.5 * workspace.detectorTwoTheta(detector)
                    qvalue = (4*np.pi/wavelength)*np.sin(usignTheta)  # in inverse Angstroms
                    qvalues.append(qvalue)
            except:
                raise RuntimeError("Unable to compute the Q values")
        return qvalues

    def PyExec(self):
        # Gather input
        qens_data = self.getProperty("InputWorkspace").value
        # Find the Q values
        try:
            self._qvalues = self._extract_qvalues(qens_data)
        except RuntimeError as error_message:
            self.log().error(str(error_message))
            if self.getPropertyValue("RaiseMode"):
                raise RuntimeError(error_message)
            self._qvalues = list() # empty list
        # Write output
        self.setProperty("Qvalues", self._qvalues)

AlgorithmFactory.subscribe(GetQsInQENSData)
