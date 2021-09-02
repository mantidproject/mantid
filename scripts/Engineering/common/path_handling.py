# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path
from mantid.kernel import logger
from mantid.simpleapi import Load


def get_run_number_from_path(run_path, instrument):
    return path.splitext(path.basename(run_path))[0].replace(instrument, '').lstrip('0')


def load_workspace(file_path):
    try:
        return Load(Filename=file_path, OutputWorkspace="engggui_calibration_sample_ws")
    except Exception as e:
        logger.error("Error while loading workspace. "
                     "Could not run the algorithm Load successfully for the data file "
                     "(path: " + str(file_path) + "). Error description: " + str(e)
                     + " Please check also the previous log messages for details.")
        raise RuntimeError
