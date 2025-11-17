
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is intended to be used in calculate spectral moments from
multidimensional workspaces. The inspiration is given by the sum rules
described in P. C. Hohenberg and W. F. Brinkman, Phys. Rev. B 10, 128 (1974).

One needs to calculate

.. math::

    I_n = \int_{-\infty}^\infty \Delta E^n S(Q,\Delta E) dE

The algorithm just multiply events by :math:`\Delta E^n`. Transforming to a multidimensional
histogram and integrating are done in different steps.


#. P. C. Hohenberg and W. F. Brinkman, Phys. Rev. B 10, 128 (1974) doi: `10.1103/PhysRevB.10.128 <https://doi.org/10.1103/PhysRevB.10.128>`_


Usage
-----

**Example - SpectralMomentMD**

.. testcode:: SpectralMomentMDExample

   # Create a workspace with one event at each integer DeltaE, from -10 to 10
   CreateMDWorkspace(EventType='MDEvent', Extents='-10,11', Names='DeltaE', Units='meV', OutputWorkspace='ws')
   FakeMDEventData(InputWorkspace='ws', UniformParams='-21')

   # Run the algorithm
   SpectralMomentMD(InputWorkspace='ws', Moment=2, OutputWorkspace='mom2')

   # Rebin
   BinMD(InputWorkspace='mom2', AlignedDim0='DeltaE, -10.5,10.5, 21', OutputWorkspace='mom2_histo')

   # Print the result
   print(mtd['mom2_histo'].getSignalArray())

Output:

.. testoutput:: SpectralMomentMDExample

  [100.  81.  64.  49.  36.  25.  16.   9.   4.   1.   0.   1.   4.   9.
    16.  25.  36.  49.  64.  81. 100.]

.. categories::

.. sourcelink::
