
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Sets up a hollow sample shape, along with the required material properties, and runs
the :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>` algorithm. This algorithm merely
serves as a simpler interface to define the shape & material of the sample without having
to resort to the more complex :ref:`CreateSampleShape <algm-CreateSampleShape-v1>` & :ref:`SetSampleMaterial <algm-SetSampleMaterial-v1>`
algorithms. The computational part is all taken care of by :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>`. Please see that
documentation for more details.

Assumptions
###########

The algorithm currently assumes that the can wall is sufficiently thin & a weak absorber so that it can be ignored.


Usage
-----

**Example**

.. testcode:: AnnularRingAbsorptionExample

   sample_ws = CreateSampleWorkspace("Histogram",NumBanks=1) # fake some data in TOF
   sample_ws = ConvertUnits(sample_ws, Target="Wavelength")
   factors = \
     AnnularRingAbsorption(sample_ws,
       SampleHeight=3.8, SampleThickness=0.05, CanOuterRadius=1.1,CanInnerRadius=0.92,
       SampleChemicalFormula="Li2-Ir-O3",SampleNumberDensity=0.004813,
       EventsPerPoint=300)

   print "The created workspace has one entry for each spectra: %i" % factors.getNumberHistograms()
   print "Just divide your data by the correction to correct for absorption."

Output:

.. testoutput:: AnnularRingAbsorptionExample

   The created workspace has one entry for each spectra: 100
   Just divide your data by the correction to correct for absorption.

.. categories::
