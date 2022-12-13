# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


class DataGenerator(object):
    def __init__(self):
        self.x_data = np.linspace(0.0, 10.0, 100)
        self.y_data = []

    def genData(self, freq, phi):
        self.y_data = np.sin(freq * self.x_data + phi)

    def getXData(self):
        return self.x_data

    def getYData(self):
        return self.y_data
