.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a helper algorithm to load data from the backscattering elastic banks for the `VISION <http://neutrons.ornl.gov/vision>`__ instrument at the SNS.


Usage
-----

**Load all of the backscattering elastic data:**

.. testcode::

   w1 = LoadVisionElasticBS("VIS_19351.nxs.h5")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput::

   Number of spectra: 20480

**Load just one of the backscattering elastic banks:**

.. testcode::

   w1 = LoadVisionElasticBS("VIS_19351.nxs.h5",Banks="bank20")

   print("Number of spectra: {}".format(w1.getNumberHistograms()))

.. testoutput::

   Number of spectra: 2048

.. categories::

.. sourcelink::
