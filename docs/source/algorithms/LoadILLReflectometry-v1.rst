.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads data of a Nexus file obtained from an ILL reflectometry instrument
`D17 <https://www.ill.eu/instruments-support/instruments-groups/instruments/d17/description/instrument-layout/>`_
or `FIGARO <https://www.ill.eu/instruments-support/instruments-groups/instruments/figaro/description/instrument-layout/>`_
into a :ref:`Workspace2D`.
Both time-of-flight and monochromatic instrument configurations are supported.
In general, this loader reads detector and monitor counts and adds x-axis and error values.
The output workspace contains histogram data.
The x-axis can have units in time-of-flight or wavelength with non-varying and varying bins, respectively.
The conversion to wavelength uses the algorithm :ref:`algm-ConvertUnits`.
Detector indices and spectrum numbers start with zero like workspace indices.

Time of flight axis
-------------------

The chopper values are used for computing the time-of-flight values for the bin edges :math:`x_i` by the following equation:

.. math:: x_{i} = \left( i + 0.5 \right) w_{\mathrm{channel}} + \Delta_{t, \mathrm{tof}} -  60^{\circ} \cdot \frac{ p_{\mathrm{off}} - 45^{\circ} + \Omega_{c2} - \Omega_{c1} + \Delta_{\mathrm{open}} }{ 2 \cdot 360^{\circ} \cdot v_{c1} } \cdot 10^{6},

with the following variables: channel width :math:`w_{\mathrm{channel}}`, time-of-flight delay :math:`\Delta_{t, \mathrm{tof}}`, offset :math:`p_{\mathrm{off}}`, phase of second chopper :math:`\Omega_{c2}`, phase of first chopper :math:`\Omega_{c1}`, open offset :math:`\Delta_{\mathrm{open}}` and velocity of first chopper :math:`v_{c1}`.

Measurement
-----------

The loader can load both types of data: direct and reflected beam.
In both cases the foreground centre will be fitted.
In case of direct beam, the detector will be rotated around the sample such that the fractional workspace index of the foreground centre will appear at 0 scattering angle.
In case of reflected beam, **BraggAngle** is mandatory, and the detector will be driven such that the foreground centre will appear at **2*BraggAngle**.

Replacing sample logs
---------------------

It is possible to replace any sample log of the loaded data, or add a new log, using the `LogsToReplace` property. The key-value pairs must be provided as JSON-compatible strings or Python
dictionaries. For an example, see the Usage section.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReflectedBeam:**

.. testcode:: LoadBraggAngle

   import numpy
   # Load ILL d17 data file (TOF mode) into a workspace 2D using a user-defined angle of 5.5 degrees:
   ws2 = LoadILLReflectometry('ILL/D17/317370.nxs', Measurement='ReflectedBeam', BraggAngle=5.5)
   detId = 202 # the foreground centre is around 202
   det = ws2.getInstrument().getDetector(detId)
   angleDet = ws2.detectorTwoTheta(det) / numpy.pi * 180

   print("Pixel at detector ID {} was rotated to {:.1f} degrees.".format(detId, angleDet))

Output:

.. testoutput:: LoadBraggAngle

   Pixel at detector ID 202 was rotated to 11.0 degrees.

.. testcleanup:: LoadBraggAngle

   AnalysisDataService.Instance().clear()

**Example - Direct Beam**

.. testcode:: LoadDirectBeam

   import numpy
   directBeamWS = LoadILLReflectometry('ILL/D17/317369.nxs')
   detId = 202 # the foreground centre is around 202
   det = directBeamWS.getInstrument().getDetector(detId)
   angleDet = directBeamWS.detectorTwoTheta(det) / numpy.pi * 180

   print("Pixel at detector ID {} was rotated to {:.1f} degrees.".format(detId, angleDet))

Output:

.. testoutput:: LoadDirectBeam

   Pixel at detector ID 202 was rotated to 0.0 degrees.

.. testcleanup:: LoadDirectBeam

   AnalysisDataService.Instance().clear()

**Example - Replace sample log**

.. testcode:: ReplaceSampleLog

   logs_to_replace = {"ChopperSetting.firstChopper": 2, "ChopperSetting.secondChopper": 1}
   ws = LoadILLReflectometry('ILL/D17/317369.nxs', LogsToReplace=logs_to_replace)

   print("The first chopper ID is: {}.".format(int(ws.getRun().getLogData("ChopperSetting.firstChopper").value)))

Output:

.. testoutput:: ReplaceSampleLog

   The first chopper ID is: 2.

.. testcleanup:: ReplaceSampleLog

   AnalysisDataService.Instance().clear()

.. categories::

.. sourcelink::
