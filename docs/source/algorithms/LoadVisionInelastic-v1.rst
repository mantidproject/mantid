.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a helper algorithm to load data from the inelastic banks for the `VISION <http://neutrons.ornl.gov/vision>`__ instrument at the SNS.


Usage
-----

**Load all of the inelastic data:**

.. testcode::

   w1 = LoadVisionInelastic("VIS_19351.nxs.h5")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput:: 

   Number of spectra: 14336

**Load just the forward inelastic banks:**

.. testcode::

   w1 = LoadVisionInelastic("VIS_19351.nxs.h5",Banks="forward")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput::

   Number of spectra: 7168

**Load the backward facing inelastic banks and the forward inelastic bank3:**

.. testcode::

   w1 = LoadVisionInelastic("VIS_19351.nxs.h5",Banks="backward,bank3")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput::

   Number of spectra: 8192

.. categories::

.. sourcelink::
