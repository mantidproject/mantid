
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the resolution of the scattering angle (theta) and
final flight path (L1) for a VESUVIO calibration provided by a PAR file.

The calculation is performed using a Monte Carlo simulation that simulates
the path of a neutron between a random point on the face of the sample to a
random point on the face of each detector.

Usage
-----

**Example - VesuvioL1ThetaResolution**

.. testcode:: VesuvioL1ThetaResolutionExample

   resolution, l1_dist, theta_dist = VesuvioL1ThetaResolution(NumEvents=1000)

   resolution_spec_names = resolution.getAxis(1).extractValues()
   print("Resolution spectra: %s" % (', '.join(resolution_spec_names)))

   print("L1 distribution spectra count: %d" % (l1_dist.getNumberHistograms()))
   print("Theta distribution spectra count: %d" % (theta_dist.getNumberHistograms()))

Output:

.. testoutput:: VesuvioL1ThetaResolutionExample

   Resolution spectra: l1_Mean, l1_StdDev, theta_Mean, theta_StdDev
   L1 distribution spectra count: 196
   Theta distribution spectra count: 196

.. categories::

.. sourcelink::
