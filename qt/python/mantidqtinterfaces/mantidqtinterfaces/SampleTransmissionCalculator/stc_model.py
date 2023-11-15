# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CalculateSampleTransmission
import numpy as np


class SampleTransmissionCalculatorModel(object):
    @staticmethod
    def calculate(input_dict):
        output_key = {}
        if input_dict["binning_type"] == 0:
            # single binning
            binning = str(input_dict["single_low"]) + "," + str(input_dict["single_width"]) + "," + str(input_dict["single_high"])
        if input_dict["binning_type"] == 1:
            # multiple binning
            binning = input_dict["multiple_bin"]

        transmission_ws = CalculateSampleTransmission(
            WavelengthRange=binning,
            ChemicalFormula=input_dict["chemical_formula"],
            DensityType=input_dict["density_type"],
            density=input_dict["density"],
            thickness=input_dict["thickness"],
        )
        output_key["x"] = transmission_ws.dataX(0)
        output_key["y"] = transmission_ws.dataY(0)
        output_key["scattering"] = transmission_ws.dataY(1)[0]
        return output_key

    @staticmethod
    def calculate_statistics(y):
        statistics = {"Min": np.min(y), "Max": np.max(y), "Mean": np.mean(y), "Median": np.median(y), "Std. Dev.": np.std(y)}
        return statistics

    @staticmethod
    def validate(input_dict):
        validation = {}
        if input_dict["binning_type"] == 0:
            # single binning
            bin_list = [input_dict["single_low"], input_dict["single_width"], input_dict["single_high"]]
            if bin_list[0] < 0.0 or bin_list[1] <= 0.0 or bin_list[2] <= 0.0:
                validation["histogram"] = "Histogram must be greater than zero."
            if bin_list[2] <= bin_list[0]:
                validation["histogram"] = "Upper histogram edge must be greater than the lower bin."
            elif bin_list[0] + bin_list[1] > bin_list[2]:
                validation["histogram"] = "Width cannot be greater than the upper bin."
        if input_dict["binning_type"] == 1:
            # multiple binning
            bin_list = input_dict["multiple_bin"].split(",")
            if bin_list:
                try:
                    bin_list = [float(i) for i in bin_list]
                    if len(bin_list) % 2 == 1 and len(bin_list) != 1:
                        for i in range(len(bin_list) // 2):
                            if bin_list[0 + 2 * i] < 0.0 or bin_list[1 + 2 * i] <= 0.0 or bin_list[2 + 2 * i] <= 0.0:
                                validation["histogram"] = "Histogram must be greater than zero."
                            if bin_list[2 + 2 * i] <= bin_list[0 + 2 * i]:
                                validation["histogram"] = "Upper histogram edge must be greater than the lower bin."
                            elif bin_list[2 * i] + bin_list[1 + 2 * i] > bin_list[2 + 2 * i]:
                                validation["histogram"] = "Width cannot be greater than the upper bin."
                    else:
                        validation["histogram"] = (
                            "Histogram requires an odd number of values. It uses the same format as the Rebin algorithm, "
                            "which is a comma separated list of first bin boundary, width, last bin boundary."
                        )
                except ValueError:
                    validation["histogram"] = "Histogram string not readable."
        if not input_dict["chemical_formula"]:
            validation["chemical_formula"] = "Chemical formula has been left blank."
        if input_dict["density"] == 0.0:
            validation["density"] = "Density can not be zero."
        if input_dict["thickness"] == 0.0:
            validation["thickness"] = "Thickness can not be zero."
        return validation
