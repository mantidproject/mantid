.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads raw data files produced by the SANS-1 instrument at MLZ into mantid workspaces.

The loader reads the detector positions from the raw data file and places the detectors accordingly <---What does this mean???

The output is a histogram workspace with a unit of wavelength (Angstrom). <--- Aren't the output dependent on the chosen "Mode"?

**Example - Loads a raw SANS-1 data file with extension .001.**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ("D0122881.001")
    numHistograms = ws.getNumberHistograms()
    print('This workspace has {0} spectra.'.format(numHistograms))

Output:

.. testoutput:: ExLoad

    This workspace has 16386 spectra.

The information from all sections of the raw data file is written to SampleLogs. To get access to
this information you simply need to use the `keys method <https://docs.mantidproject.org/nightly/tutorials/python_in_mantid/further_alg_ws/04_run_logs.html>`_;
the key consists of a section's title followed by a dot and the parameter of interest that belongs to that section.
The most relevant information, such as: position, wavelength, thickness ect. <--- Be specific about etc. is stored in separate variables.

**Example - Access to all information**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ("D0122881.001")
    run = ws.getRun()

    det_h_angle = run.getLogData('setup.DetHAngle').value   # ('section.parameter')
    wavelength = run.getLogData('wavelength').value

    print(f"DetHAngle is {det_h_angle} degrees.")
    print(f"Wavelength is {wavelength} Angstrom.")

Output:

.. testoutput:: ExLoad

    DetHAngle is 0.0 degrees.
    Wavelength is 4.9 Angstrom.

2 modes of loading data are implemented in LoadSANS1MLZ:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Vector mode (default) - counts from the loaded data file are stored as a 1D vector and written to the DataY section;
  DataX contains two columns: 1) wavelength - wavelength_spread and 2) wavelength + wavelength_spread; with this mode workspace has
  `IDF <https://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html>`_ ‘SANS-1’. <-- What do you mean "has"?

* Matrix mode - counts from the loaded data file are stored as a 2D matrix (128, 128) and written to the DataY section <--- Needs to be paraphrased
  (this allows `plot functions <https://docs.mantidproject.org/nightly/tutorials/mantid_basic_course/loading_and_displaying_data/04_displaying_2D_data.html>`_);
  DataX will contain x position for each pixel of the detector; with this mode workspace doesn’t have IDF. <---again, what is "doesn't have"



**Example - Load a raw data file with Matrix mode**

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
