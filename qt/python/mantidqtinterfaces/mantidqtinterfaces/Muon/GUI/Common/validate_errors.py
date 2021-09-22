# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


def validateToErrors(alg):
    errors = alg.validateInputs()
    if not errors:
        return
    message = "Invalid properties found: \n"
    for key in errors.keys():
        message += key + ": " + errors[key] + "\n"
    raise ValueError(message)
