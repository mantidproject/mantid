.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometryISISCalibration
-------------------------------

This algorithm adjusts the positions of individual detector pixels in a workspace based on the calibration values provided in the calibration file. The algorithm takes a copy of the input workspace before applying the position changes, so the original workspace remains unchanged.

A ``.dat`` calibration file must be provided via the ``CalibrationFile`` property. Lines beginning with ``#`` are ignored.

The ``CalibrationAngleType`` property controls the meaning of the calibration values:

- ``Offset``: the file should contain two, space-delimited columns labelled ``detectorid`` and ``theta_offset`` (the labels are not case-sensitive). The detector IDs are the IDs for the detector pixels that should be moved. The theta offsets are the change required to the two theta value for each detector, in degrees.
- ``Absolute``: the file should contain two, space-delimited columns labelled ``detectorid`` and ``angle``. The data rows are treated as an ordered list of detector index and absolute angle values. In this mode the first column is used for ordering only; detector IDs are resolved from the non-monitor detector spectra in the input workspace. The absolute angle values are converted to two theta offsets internally.

For absolute calibration files, ``AbsoluteAngleType`` specifies whether the supplied absolute values are ``Theta`` or ``TwoTheta`` values. ``Theta`` values are converted to detector two theta by applying a factor of two. ``CalibrationSpecularPixelIndex`` gives the specular pixel index in the calibration file, and ``ExperimentSpecularPixelIndex`` gives the specular pixel index in the input workspace. These indices may be fractional, in which case linear interpolation is used.

The ``DetectorCorrectionType`` property is passed to :ref:`algm-SpecularReflectionPositionCorrect` and controls whether pixels are moved by a vertical shift or rotated around the sample.

The below is an example of a valid calibration file:

.. code-block:: none

    # Optional header that can be used
    # to include any useful information
    detectorid theta_offset
    1 -0.05
    2 -0.043
    3 0.02

For each detector pixel in the calibration file, the algorithm finds the existing two theta value in degrees and then adds the theta offset to calculate the new two theta value (so new two theta = existing two theta + theta offset). This new two theta is then passed to :ref:`algm-SpecularReflectionPositionCorrect`, which converts it to cartesian co-ordinates and moves the detector pixel to the new location.

Only the detectors specified in the file will be moved, all other detectors in the workspace will remain in their original positions.

The below is an example of a valid absolute-angle calibration file:

.. code-block:: none

    detectorid angle
    0 0.45
    1 0.47
    2 0.49

For absolute input, all non-monitor detector spectra in the input workspace are considered in workspace-index order.

Usage
-------

.. include:: ../usagedata-note.txt

**Example: Run calibration on a workspace**

.. testcode:: ExCalibration

    import os
    import numpy as np

    # Load an empty workspace
    ws = LoadEmptyInstrument(InstrumentName='D17')
    detector_info = ws.detectorInfo()
    print(f"Before calibration:")
    for i in range(1,4):
        two_theta = np.rad2deg(detector_info.twoTheta(detector_info.indexOf(i)))
        print(f"Detector ID {i} two theta: {two_theta}")

    # Create a calibration file to change the first 3 detectors
    file_name = os.path.join(config["defaultsave.directory"], "TestReflectometryISISCalibration.dat")
    f = open(file_name,'w');
    f.write('DetectorID Theta_Offset\n')
    for i in range(1,4):
       f.write(f'{i} 0.1\n')
    f.close();

    # Run calibration
    calibrated_ws = ReflectometryISISCalibration(InputWorkspace=ws, CalibrationFile=file_name)
    detector_info = calibrated_ws.detectorInfo()
    print(f"After calibration:")
    for i in range(1,4):
        two_theta = np.rad2deg(detector_info.twoTheta(detector_info.indexOf(i)))
        print(f"Detector ID {i} two theta: {two_theta}")

Output:

.. testoutput:: ExCalibration

    Before calibration:
    Detector ID 1 two theta: 2.791743331094234
    Detector ID 2 two theta: 2.7697087152076083
    Detector ID 3 two theta: 2.7476732793761274
    After calibration:
    Detector ID 1 two theta: 2.891743331094271
    Detector ID 2 two theta: 2.869708715207532
    Detector ID 3 two theta: 2.847673279376101

.. testcleanup:: ExCalibration

    import os
    os.remove(file_name)

.. seealso :: Algorithm :ref:`algm-ReflectometryISISLoadAndProcess`, :ref:`algm-SpecularReflectionPositionCorrect`.

.. categories::

.. sourcelink::
