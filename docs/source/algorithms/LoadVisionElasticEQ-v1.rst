.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a helper algorithm to load data from the equatorial elastic banks for the `VISION <http://neutrons.ornl.gov/vision>`__ instrument at the SNS.


Usage
-----

**Load all of the equatorial elastic data:**

.. testcode::

   w1 = LoadVisionElasticEQ("VIS_19351.nxs.h5")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput::

   Number of spectra: 12288

**Load just one of the equatorial elastic banks:**

.. testcode::

   w1 = LoadVisionElasticEQ("VIS_19351.nxs.h5",Banks="bank29")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput::

   Number of spectra: 2048

.. categories::

.. sourcelink::
