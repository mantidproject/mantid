# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

# This class is the ideal tube, which specifies where the peaks formed by slits or edges should occur

# Author: Karl Palmen ISIS


class IdealTube(object):
    """
    The IdealTube specifies where the peaks formed by slits or edges should occur.

    They can be considered as the known positions as well.
    It does also keep informatin about the shape of the known_positions, if they are
    peaks or edges.

    First you create an ideal tube by calling the empty constructor idealTube().
    Then call another function to populate it.

    You may fill it up with the following methods:

     * :meth:`~ideal_tube.IdealTube.setArray`
     * :meth:`~ideal_tube.IdealTube.setPositionsAndForm`

    You may than query the known positions and functional forms through :

     * :meth:`~ideal_tube.IdealTube.getArray`
     * :meth:`~ideal_tube.IdealTube.getFunctionalForms`

    """

    def __init__(self):
        """
        Create empty instance
        """
        self.positions = np.empty(0, dtype=float)  # position of the points in metres
        self.functionalForms = []  # function form of points 1=peak 2=edge. peaks assumed if [].

    def setArray(self, array):
        """
        Construct an ideal tube directly from an array of positions

        :param array: Array of points where the peaks should be in Metres

        """
        self.positions = np.array(array)

    def setForm(self, form):
        """Define the functional form for the peaks"""
        self.functionalForms = form

    def setPositionsAndForm(self, pos, form):
        """
        Construct and ideal tube directly from an array of positions and functional forms

        :param pos: Array of points where the peaks or edges should be in Metres
        :param form: Array of functional forms of the points 1=peak, 2=edge

        """
        self.positions = np.array(pos)
        self.functionalForms = form

    def constructTubeFor3PointsMethod(self, idealAP, idealBP, idealCP, activeTubeLen):
        """
        Construct and ideal tube for Merlin 3-point calibration

        :param idealAP: Ideal left (AP) in pixels
        :param idealBP: ideal right (BP) in pixels
        :param idealCP: ideal centre (CP) in pixels
        :param activeTubeLen: Active tube length in metres

        """
        # Construct Ideal tube for 3 point calibration of MERLIN standard tube (code could be put into a function)
        pixelLen = activeTubeLen / 1024  # Pixel length

        # we then convert idealAP, idealCP and idealBP to Y coordinates and put into ideal tube array
        self.positions = np.array(
            [idealAP * pixelLen - activeTubeLen / 2, idealCP * pixelLen - activeTubeLen / 2, idealBP * pixelLen - activeTubeLen / 2]
        )
        self.functionalForms = [2, 1, 2]

    def getArray(self):
        """
        Return the array of of points where the peaks should be in Metres
        """
        return self.positions

    def getFunctionalForms(self):
        """
        Return the array of of points where the peaks should be in Metres
        """
        return self.functionalForms
