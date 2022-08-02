.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This loads the raw data files produced by the SANS-1 instruments at the MLZ.

This loader reads the detector positions from the raw data file and places the detectors accordingly.

The output is a histogram workspace with unit of wavelength (Angstrom).

**Example - Loads a 001 raw data file.**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ("D0511339.001")
    numHistograms = ws.getNumberHistograms()
    print('This workspace has {0} spectra.'.format(numHistograms))

Output:

.. testoutput:: ExLoad

    This workspace has 16386 spectra.

All information from all sections that contains in raw data file rescheduled to SampleLogs. To get access to
this information you simply need to using the `keys method <https://docs.mantidproject.org/nightly/tutorials/python_in_mantid/further_alg_ws/04_run_logs.html>`_;
key consist of section name and variable in that section.
Also main information such as: position, wavelength, thickness etc. are stored in separate variables.

**Example - Access to all information**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ("D0511339.001")
    run = ws.getRun()

    det_h_angle = run.getLogData('setup.DetHAngle').value   # ('section.variable')
    wavelength = run.getLogData('wavelength').value

    print(f"DetHAngle is {det_h_angle} degrees.")
    print(f"Wavelength is {wavelength} Angstrom.")

Output:

.. testoutput:: ExLoad

    DetHAngle is 0.0 degrees.
    Wavelength is 6.0 Angstrom.

.. include:: ../usagedata-note.txt

.. categories::

.. sourcelink::
