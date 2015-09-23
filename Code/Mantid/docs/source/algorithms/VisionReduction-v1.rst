.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm reduces for data from the inelastic banks for the `VISION <http://neutrons.ornl.gov/vision>`__ instrument at the SNS..

Usage
-----

**Perform a reduction:**

.. testcode::

   w1 = VisionReduction("VIS_19351.nxs.h5")

   print "Number of spectra:", w1.getNumberHistograms()

.. testoutput:: 

   Number of spectra: 3


.. categories::

.. sourcelink::
