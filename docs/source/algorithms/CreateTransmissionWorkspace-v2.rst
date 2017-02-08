.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a transmission run workspace given one or two TOF workspaces.
If two workspaces are provided, then the first workspace is considered
a low wavelength transmission run, and the second workspace is considered
a high wavelength transmission run.

Both input workspaces must have X-units of TOF. They are first converted
to units of wavelength and then stitched together using :ref:`algm-Stitch1D`,
:literal:`Params`, :literal:`StartOverlap` and :literal:`EndOverlap`. A single
output workspace is generated with X-units of wavelength in angstroms.


.. diagram:: CreateTransmissionWorkspace_HighLvl-v2_wkflw.dot

The diagram above illustrates the main steps in the algorithm. Below is a more
detailed diagram describing how transmission workspaces are converted to units
of wavelength and normalized by monitors. First, detectors and monitors are
extracted from the input workspace using :ref:`algm-GroupDetectors` and
:ref:`algm-CropWorkspace` respectively, using ``ProcessingInstructions`` in the
first case and ``I0MonitorIndex`` in the second case. Then, each of the resulting
workspaces is converted to wavelength (note that ``AlignBins`` is set to ``True``
for this), detectors are normalized by monitors, and the resulting workspace is
cropped in wavelength according to ``WavelengthMin`` and ``WavelengthMax``, which
are both mandatory parameters. Note that the normalization by monitors is optional,
and only takes place if :literal:`I0MonitorIndex`, :literal:`MonitorBackgroundWavelengthMin`
and :literal:`MonitorBackgroundWavelengthMax` are provided. In this case, the monitor
of interest will be extracted from the input workspace, converted to wavelength, and its
background will be subtracted according to :literal:`MonitorBackgroundWavelengthMin`
and :literal:`MonitorBackgroundWavelengthMax`. If :literal:`MonitorIntegrationWavelengthMin`
and :literal:`MonitorIntegrationWavelengthMax` are provided, monitors will be integrated
according to that range. If monitors are not integrated, there is an addition step in which
detectors will be rebinned to monitors using :ref:`algm-RebinToWorkspace`, to ensure that
the normalization can be performed. Below is a summary of the main steps in the algorithm.
For the sake of clarity, all possible steps are illustrated, even if some of them are
optional.

.. diagram:: CreateTransmissionWorkspace_ConvertToWavelength-v2_wkflw.dot

Previous Versions
-----------------

This is version 2 of the algorithm. For version 1, please see `here. <CreateTransmissionWorkspace-v1.html>`_

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Create a transmission run**

.. testcode:: ExCreateTransWSSimple

    trans = Load(Filename='INTER00013463.nxs')
    transWS = CreateTransmissionWorkspace(FirstTransmissionRun = trans,
                                          I0MonitorIndex = 2,
                                          ProcessingInstructions = '3,4',
                                          WavelengthMin = 1,
                                          WavelengthMax = 17,
                                          MonitorBackgroundWavelengthMin = 15,
                                          MonitorBackgroundWavelengthMax = 17,
                                          MonitorIntegrationWavelengthMin = 4,
                                          MonitorIntegrationWavelengthMax = 10)

    print "The first four transWS Y values are:"
    for i in range (4):
        print "%.4f" % transWS.readY(0)[i]

Output:

.. testoutput:: ExCreateTransWSSimple

    The first four transWS Y values are:
    0.0052
    0.0065
    0.0088
    0.0123

**Example - Create a transmission run from two runs**

.. testcode:: ExCreateTransWSTwo

    trans1 = Load(Filename='INTER00013463.nxs')
    trans2 = Load(Filename='INTER00013464.nxs')
    transWS = CreateTransmissionWorkspace(FirstTransmissionRun = trans1,
                                          SecondTransmissionRun = trans2,
                                          Params = [1.5,0.02,17],
                                          StartOverlap = 10.0,
                                          EndOverlap = 12.0,
                                          I0MonitorIndex = 2,
                                          ProcessingInstructions = '3,4',
                                          WavelengthMin = 1,
                                          WavelengthMax = 17,
                                          MonitorBackgroundWavelengthMin = 15,
                                          MonitorBackgroundWavelengthMax = 17,
                                          MonitorIntegrationWavelengthMin = 4,
                                          MonitorIntegrationWavelengthMax = 10)

    print "The first four transWS Y values are:"
    for i in range (4):
        print "%.4f" % transWS.readY(0)[i]

Output:

.. testoutput:: ExCreateTransWSTwo

    The first four transWS Y values are:
    0.0563
    0.0561
    0.0570
    0.0578

.. categories::

.. sourcelink::
