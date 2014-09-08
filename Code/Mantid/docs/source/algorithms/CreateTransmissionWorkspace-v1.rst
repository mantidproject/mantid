.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a transmission run workspace given one or two TOF workspaces,
and the original run workspace. If two workspaces are provided, then the
first workspace is considered a low wavelength transmission run, and the
second workspace is considered a high wavelength transmission run.

The two transmission run workspaces are converted to IvQ workspaces and
then stitched together using :ref:`algm-Stitch1D`.
Both input workspaces must have x-units of TOF.

A single output workspace is generated with x-units of Wavelength in angstroms.

In most cases you will want to use :ref:`algm-CreateTransmissionWorkspaceAuto`,
which is a facade over this algorithm that correctly configures the input
properties for you.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Create a transmission run**

.. testcode:: ExCreateTransWSSimple

    trans = Load(Filename='INTER00013463.nxs')
    transWS = CreateTransmissionWorkspace(
      FirstTransmissionRun = trans,
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
    0.0160
    0.0348
    0.0908
    0.1187

**Example - Create a transmission run from two runs**

.. testcode:: ExCreateTransWSTwo

    trans1 = Load(Filename='INTER00013463.nxs')
    trans2 = Load(Filename='INTER00013464.nxs')
    transWS = CreateTransmissionWorkspace(
      FirstTransmissionRun = trans1,
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
    0.0564
    0.0569
    0.0582

.. categories::
