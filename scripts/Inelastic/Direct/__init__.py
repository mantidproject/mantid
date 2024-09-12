# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Classes and functions exported from Direct Reduction Package:

dgreduce               -- outdated functions, provided for compartibility with old qtiGenie
DirectEnergyConversion -- main reduction class
PropertyManager        -- helper class providing access and functionality for IDF properties
ReductionWrapper       -- parent class for reduction, After overloading for particular insrument
                          provides common interface for Mantid and web services
RunDescriptor          -- Class descripbing a run and instantiated as property of Property manager to specify a
                          particular kind of run (e.g. sample run, monovan run etc...)

"""

__all__ = ["dgreduce", "DirectEnergyConversion", "PropertyManager", "ReductionWrapper", "RunDescriptor"]
