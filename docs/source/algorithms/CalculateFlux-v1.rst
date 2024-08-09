
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the incident flux using an empty beam measurement for a SANS instrument in reactor sources.

Input
-----

Input must be a workspace corresponding to an empty beam measurement.
It needs to have the same number of bins for all the spectra, but it does not have to have exact same axes.
The X-axis unit can be either wavelength (for TOF) or dimensionless.
In case of TOF, the algorithm will take the global minimum and the global maximum of the wavelengths in the input workspace, and make equidistant bins such that it has the same number of bins as the input.
The input has to be already normalised in the same fashion, as the sample workspaces that are going to be normalised with the flux calculated with this algorithm.
For example, if the input is normalised by time, the flux will mean number of neutrons per second.

Output
------

The output will contain the flux as a function of the wavelength.

Flux
----

For the given wavelength bin, the flux is the sum of all the counts corresponding to the pixels that are within a cylinder around the z axis with the radius of **BeamRadius**.

Usage
-----

**Example - CalculateFlux**

.. testcode:: CalculateFlux

  ws = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, Function="One Peak")
  flux = CalculateFlux(InputWorkspace=ws, BeamRadius=0.05)
  print("{:.1f}".format(flux.readY(0)[5]))

Output:

.. testoutput:: CalculateFlux

  11.1

.. categories::

.. sourcelink::
