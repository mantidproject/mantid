.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads the raw data files produced by the SANS-1 instrument at MLZ into mantid workspace.

The loader reads out the detector positions from the instrument definition file
(`IDF <https://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html>`_) and places the
detectors accordingly. The `Instrument View <https://www.mantidproject.org/Instrument_View>`_ window
can be activated by right clicking on the loaded workspace and selecting the "Show Instrument" option
(available only in a :ref:`"vector" <modes>` mode).

.. _vector example:
**Example - Loads raw SANS-1 data file with extension .001.**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ("D0122881.001")
    numHistograms = ws.getNumberHistograms()
    print('This workspace has {0} spectra.'.format(numHistograms))

Output:

.. testoutput:: ExLoad

    This workspace has 16386 spectra.

The information from all sections of the raw data file is written to Sample Logs. To get access to
Sample Logs, a user needs to use the
`keys method <https://docs.mantidproject.org/nightly/tutorials/python_in_mantid/further_alg_ws/04_run_logs.html>`_;
the key consists of a section's title followed by a dot and the parameter of interest that belongs
to that section. The most relevant parameters, such as "wavelength", "collimation",
"sample_detector_distance", "duration", "monitor1", "monitor2", "thickness" and "position" can be accessed
directly using the parameter name.

**Example - Access to the information**

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

.. _modes:
Two modes of loading data
-------------------------

* Whenever the loader is used in a ``vector`` (default) mode  then the output
  is written to a 1D vector-shaped workspace. As a result, the data can be visualised by right
  clicking on the loaded workspace and selecting the "Show Instrument" option, which activates the
  `Instrument View <https://www.mantidproject.org/Instrument_View>`_ tool. Also, by selecting the
  "Show Detectors" option, you can activate the Detector Table, which contains Workspace Indices,
  Spectrum Numbers, Detector IDs and locations of the detectors, together with a flag showing if
  a detector is a monitor.

* Whenever the loader is used in a ``matrix`` mode, then the output is written
  to a 2D matrix workspace. In this case, the data can be visualised by right clicking on the loaded
  workspace and selecting `Show Slice Viewer <https://docs.mantidproject.org/nightly/tutorials/mantid_basic_course/loading_and_displaying_data/04_displaying_2D_data.html>`_
  or one of the options under the
  `Plot <https://docs.mantidproject.org/nightly/tutorials/mantid_basic_course/loading_and_displaying_data/04_displaying_2D_data.html>`_
  menu. At the same time, the "Show Instrument" and "Show Detectors" options will not be accessible.

.. _matrix example:
**Example - Load a raw data file with Matrix mode**

.. testcode:: ExLoad

    ws = LoadSANS1MLZ(Filename='D0122881.001',
                      Mode='matrix')
    spectrum_5 = ws.readY(5)
    print(spectrum_5[1:6])

Output:

.. testoutput:: ExLoad

    [891. 863. 890. 836. 885.]

.. include:: ../usagedata-note.txt

.. categories::

.. sourcelink::
