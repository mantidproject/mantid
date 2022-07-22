.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the specialized loader for the raw `.nxs` files produced by the reflectometry instruments at ILL.
Currently it supports `D17 <https://www.ill.eu/instruments-support/instruments-groups/instruments/d17/description/instrument-layout/>`_ and `FIGARO <https://www.ill.eu/instruments-support/instruments-groups/instruments/figaro/description/instrument-layout/>`_. This loader can load only a single file at each call. If loading more than one
file is required, please refer to :ref:`Load <algm-Load>` or :ref:`LoadAndMerge <algm-LoadAndMerge>` algorithms, which are
more suited for the task.

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
In case of reflected beam, if **BraggAngle** is provided, then it will be the angle used for the rotation of the detector such that the foreground centre will appear at **2*BraggAngle**.
If the **BraggAngle** property is not specified, then the sample angle metadata position (`SAN`)` is going to be used instead.

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

.. categories::

.. sourcelink::
