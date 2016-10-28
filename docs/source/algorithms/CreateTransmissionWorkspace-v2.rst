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
of wavelength and normalized by monitors. First, the input workspace is converted
to wavelength and cropped according to :literal:`WavelengthMin` and `WavelengthMax`.
Next, :literal:`ProcessingInstructions` are used as the grouping pattern to extract
the detectors of interest. If :literal:`I0MonitorIndex`, :literal:`MonitorBackgroundWavelengthMin`
and :literal:`MonitorBackgroundWavelengthMax` are provided the monitor will be
extracted from the input workspace and its background will be subtracted according
to :literal:`MonitorBackgroundWavelengthMin` and :literal:`MonitorBackgroundWavelengthMax`.
If :literal:`MonitorIntegrationWavelengthMin` and :literal:`MonitorIntegrationWavelengthMax`
are provided, monitors will be integrated according to that range. Finally, the
detector workspace will be normalize by the monitor workspace. Note that monitor
normalization is optional.

.. diagram:: CreateTransmissionWorkspace_ConvertToWavelength-v2_wkflw.dot

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
    0.0255
    0.0759
    0.1324
    0.1424

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
    0.0573
    0.0575
    0.0585
    0.0585

.. categories::

.. sourcelink::
