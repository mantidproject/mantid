
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the minimum and maximum momentum transfer (Q) for the workspace.
The input workspace must have instrument defined and data in units of wavelength [Angstroms].
Masked detectors and monitors do not enter the calculation.
The calculated values (in inverse Angstroms) will be set in sample logs as **qmin** and **qmax** respectively.

Usage
-----

**Example - CalculateQMinMax**

.. testcode:: CalculateQMinMaxExample

  ws = CreateSampleWorkspace(XUnit='Wavelength', NumBanks=1, PixelSpacing=0.1, XMin=1, XMax=5, BinWidth=0.4)
  MoveInstrumentComponent(Workspace=ws, RelativePosition=True, ComponentName="bank1", Y=-0.5, X=-0.5)
  shapeXML = \
  """
  <infinite-cylinder id="A" >
  <centre x="0" y="0" z="0" />
  <axis x="0" y="0" z="1" />
  <radius val="0.1" />
  </infinite-cylinder>
  """
  MaskDetectorsInShape(ws, ShapeXML=shapeXML)
  CalculateQMinMax(Workspace=ws)
  print("QMin = %.5f" % ws.getRun().getLogData("qmin").value)
  print("QMax = %.5f" % ws.getRun().getLogData("qmax").value)

Output:

.. testoutput:: CalculateQMinMaxExample

  QMin = 0.03553
  QMax = 0.88200

.. categories::

.. sourcelink::
