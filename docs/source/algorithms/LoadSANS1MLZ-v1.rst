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

    ws = LoadSANS1MLZ("D0122881.001")
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

    ws = LoadSANS1MLZ("D0122881.001")
    run = ws.getRun()

    det_h_angle = run.getLogData('setup.DetHAngle').value   # ('section.variable')
    wavelength = run.getLogData('wavelength').value

    print(f"DetHAngle is {det_h_angle} degrees.")
    print(f"Wavelength is {wavelength} Angstrom.")

Output:

.. testoutput:: ExLoad

    DetHAngle is 0.0 degrees.
    Wavelength is 4.9 Angstrom.

LoadSANS1MLZ has 2 mods to load data:
~~~~~~~~~~~

* Vector mode (default) - counts from data file will be reshaped to 1 dimensional matrix and written to DataY section;
  DataX contains wavelength - wavelength_error and  wavelength + wavelength_error; with this mode workspace has
  `IDF <https://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html>`_ ‘SANS-1’.

* 128x128 mode - counts from data file will be reshaped to (128, 128) dimensional matrix and write to DataY section
  (this allows `plot functions <https://docs.mantidproject.org/nightly/tutorials/mantid_basic_course/loading_and_displaying_data/04_displaying_2D_data.html>`_);
  DataX will contain x position for each pixel of the detector; with this mode workspace doesn’t have IDF.



**Example - Load a raw datafile with 128x128 mode**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ(Filename='D0122881.001',
                      Mode='128x128')
    spectrum_5 = ws.readY(5)
    print(spectrum_5[1:6])

Output:

.. testoutput:: ExLoad

    [891. 863. 890. 836. 885.]

.. include:: ../usagedata-note.txt

.. categories::

.. sourcelink::
