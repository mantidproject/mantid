.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes a range of input runs that have been obtained using the Sample Changer on Indirect Inelastic Instruments at the ISIS facility.
The sample changer has 3 positions. Input run numbers are associated with certain general temperature points. The algorithm uses the position of the
sample to determine which specific temperature from the sample logs should be used for an `ElasticWindowScan <algm-ElasticWindowScan>`.
From the ElasticWindowScan, the EISF (elastic incoherent scattering factor) is calculated and there is an option to run :ref:`MSDFit <algm-MSDFit>`.
This is repeated for the number of different materials in the sample changer.

For example, if there are 2 different materials in the sample changer, at 3 different temperatures 10K, 20K and 300K.
The algorithm would execute 2 scans, one for each sample:
The first scan would be 1st, 3rd and 5th runs combined.
The second scan would be 2nd, 4th, and 6th runs combined.

.. categories::

.. sourcelink::
