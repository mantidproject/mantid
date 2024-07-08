# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR WISH
#
# Wish instrument uses a single run for each panel. K

"""WISH instrument scientists run a single acquisition per panel to calibrate their instrument.
So, for the current status of WISH with 5 panels, it is necessary 5 runs to calibrate it.

This examples show how you can use these runs to produce a single calibration table that can be use
to calibrate the whole instrument.

"""

import numpy
import mantid.simpleapi as mantid

import tube
from tube_spec import TubeSpec


def CalibrateWish(run_per_panel_list):
    """
    :param run_per_panel_list: is a list of tuples with the run number and the associated panel

    run_per_panel_list =  [ (17706, 'panel01'), (17705, 'panel02'),  (17701, 'panel03'), (17702, 'panel04'), (17695, 'panel05')]
    """
    # == Set parameters for calibration ==
    previousDefaultInstrument = mantid.config["default.instrument"]
    mantid.config["default.instrument"] = "WISH"

    # definition of the parameters static for the calibration
    lower_tube = numpy.array([-0.41, -0.31, -0.21, -0.11, -0.02, 0.09, 0.18, 0.28, 0.39])
    upper_tube = numpy.array(lower_tube + 0.003)
    funcForm = 9 * [1]  # 9 gaussian peaks
    margin = 15
    low_range = list(range(0, 76))
    high_range = list(range(76, 152))
    kwargs = {"margin": margin}

    # it will copy all the data from the runs to have a single instrument with the calibrated data.
    whole_instrument = mantid.LoadRaw(str(run_per_panel_list[0][0]))
    whole_instrument = mantid.Integration(whole_instrument)

    for run_number, panel_name in run_per_panel_list:
        panel_name = str(panel_name)
        run_number = str(run_number)
        # load your data and integrate it
        ws = mantid.LoadRaw(run_number, OutputWorkspace=panel_name)
        ws = mantid.Integration(ws, 1, 20000, OutputWorkspace=panel_name)

        # use the TubeSpec object to be able to copy the data to the whole_instrument
        tube_set = TubeSpec(ws)
        tube_set.setTubeSpecByString(panel_name)

        # update kwargs argument before calling calibrate
        kwargs["rangeList"] = low_range  # calibrate only the lower tubes
        calibrationTable = tube.calibrate(ws, tube_set, lower_tube, funcForm, **kwargs)

        # update kwargs
        kwargs["calibTable"] = calibrationTable  # append calib to calibrationtable
        kwargs["rangeList"] = high_range  # calibrate only the upper tubes

        calibrationTable = tube.calibrate(ws, tube_set, upper_tube, funcForm, **kwargs)
        kwargs["calibTable"] = calibrationTable

        mantid.ApplyCalibration(ws, calibrationTable)

        # copy data from the current panel to the whole_instrument
        for i in range(tube_set.getNumTubes()):
            for spec_num in tube_set.getTube(i):
                whole_instrument.setY(spec_num, ws.dataY(spec_num))

    # calibrate the whole_instrument with the last calibrated panel which has the calibration accumulation
    # of all the others
    mantid.CopyInstrumentParameters(run_per_panel_list[-1][1], whole_instrument)

    mantid.config["default.instrument"] = previousDefaultInstrument
    # ==== End of CalibrateWish() ====


if __name__ == "__main__":
    # this file is found on cycle_11_1
    run_per_panel_list = [(17706, "panel01"), (17705, "panel02"), (17701, "panel03"), (17702, "panel04"), (17695, "panel05")]
    CalibrateWish(run_per_panel_list)
