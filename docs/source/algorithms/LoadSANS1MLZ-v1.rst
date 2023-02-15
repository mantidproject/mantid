.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads the raw data files produced by the SANS-1 instrument at MLZ into mantid workspace.

The loader reads out the detector positions from the instrument definition file
(:ref:`IDF <InstrumentDefinitionFile>`) and places the detectors accordingly.
The :ref:`Instrument View <InstrumentViewer>` window
can be activated by right clicking on the loaded workspace and selecting the "Show Instrument".

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
Sample Logs, a user needs to use the :ref:`keys method <04_run_logs>`;
the key consists of a section's title followed by a dot and the parameter of interest that belongs
to that section. The most relevant parameters, such as "wavelength", "collimation",
"l2", "duration", "monitor1", "monitor2", "thickness" and "position" can be accessed
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

Data visualization
-------------------------

Whenever the loader is used then the output is written to a 2D Workspace. As a result, the data
can be visualised by right clicking on the loaded workspace and selecting the "Show Instrument"
option, which activates the `Instrument View <InstrumentViewer>`_ tool.

.. figure:: /images/LoadSANS1MLZ_show_instrument.png
    :align: center
    :width: 1000

By selecting the "Show Detectors" option, you can activate the Detector Table, which contains
Workspace Indices, Spectrum Numbers, Detector IDs and locations of the detectors, together with
a flag showing if a detector is a monitor.


.. figure:: /images/LoadSANS1MLZ_show_detectors.png
    :align: center
    :width: 1400

.. include:: ../usagedata-note.txt

.. categories::

.. sourcelink::
