.. _Tube_calib:

Tube calib
==========


The file tube_calib.py is the main python file for tube calibration. It provides the function getCalibration.

getCalibration
--------------

The getCalibration function gets the calibration and puts it into a calibration table. It has the following arguments:


===== ================== ====================== ========== ==========================================================
Order Name               Type                   Default    Description
===== ================== ====================== ========== ==========================================================
1     ws                 Workspace              Mandatory  Workspace with tubes to be calibrated
2     tubeSet            TubeSpec               Mandatory  Specifies which tubes to calibrate to provide detector info for calibration
3     calibTable         TableWorkspace         Mandatory  An empty table workspace with int column 'Detector ID' and a V3D column 'Detector Position'. It will be filled with the IDs and calibrated positions of the detectors.
4     fitPar             TubeCalibFitParams     Mandatory  An object bearing the tube fitting parameters.
5     iTube              IdealTube              Mandatory  Contains the positions in meters of the shadows of the slits, bars or edges used for calibration.
6     PeakTestMode       boolean                False      If True, will move away each detector located at a reckoned shadow to make it visible, for testing purposes.
7     OverridePeaks      Array of real numbers  []         If non-zero length, an array of shadow positions in pixels to override those that would be fitted for one tube
8     PeaksFile          string                 ""         If non-zero length, the name of a file to put the shadow positions in pixels
9     ExcludeShortTubes  real number            0.0        Excludes tubes shorter than the value given in meters from calibration (only used for MERLIN)
===== ================== ====================== ========== ==========================================================

Other python code exists to help create the arguments. They consist of the classes :ref:`TubeSpec` and :ref:`TubeCalibFitParams` and ``IdealTube``.
