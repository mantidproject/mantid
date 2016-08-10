.. _Engineering_Diffraction_TestGuide-ref:

Engineering Diffraction Testing
=================================

.. contents:: Table of Contents
    :local:
    
Preamble
^^^^^^^^^
This document is tailored towards developers intending to test the Engineering Diffraction
interface. Whilst users may benefit from the extended examples provided in this document
it is recommended they read :ref:`Engineering_Diffraction-ref:` which is tailored towards users.

The files used in the following examples are found within System test files within 
`<MantidBuildDir>/ExternalData/Testing/Data/SystemTest` after building the *SystemTestData* target.

Overview
^^^^^^^^
The Engineering Diffraction interface allows scientists using the EnginX instrument to interactively
process their data. There are 5 tabs from which are ordered according to the main steps performed.
These are:

- Calibration - This is where a vanadium and cerium run are entered to calibrate the subsequent data.
- Focus - This is where the data can be masked to only use specific detectors or all.
- Pre-processing - Event workspaces have to be converted into histogram data before 
  focusing which can be completed here.
- Fitting - The user uses their focused data here to interactively pick peaks for subsequent fitting.
- Settings - This holds additional settings which may be of use during testing.

Calibration
^^^^^^^^^^^
The first time the interface is opened it will be disabled except for the RB number entry box
and a pop up box which should prompt for a RB number. For testing purposes this can be anything.

If a vanadium calibration has not previously been run the `Current calibration` will be empty
and the following files need selected and calibration run.
- *Vanadium#* ENGINX00236516.nxs
- *Calibration#* ENGINX00194547.nxs

If `Current calibration` is populated the algorithm will attempt to use a cached calculation instead
of recalculating the calibration. To force it to recalculate every time for testing purposes the
option can be set under :ref:`settings-Engineering_Diffraction-ref:`.




.. _settings-Engineering_Diffraction_TestGuide-ref

Settings
^^^^^^^^^