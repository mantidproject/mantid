.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even
   removed without a notification, should instrument scientists decide to do so.

This algorithm computes detector efficiency correction coefficients using a given Vanadium and background workspaces. As a result, output workspace or group of workspaces with coefficients will be created.

Detector efficiency correction is performed using the measurements of vanadium standard sample (hereafter Vanadium). Background for Vanadium must be also measured and provided to the algorithm as an input **BkgWorkspace**. Vanadium and its background can be measured at several detector bank positions.  This algorithm does the detector efficiency for a particular run in following steps:

.. warning::

    Algorithm requires data normalized either to monitor counts or experiment duration. The normalization must be the same for all input workspaces.

1. Subtract Background from Vanadium:

   :math:`V_i = (V_i)_{raw} - B_i`

   The :ref:`algm-Minus` algorithm is used for this step. In the case of negative result, the error message will be produced and the algorithm terminates.

2. Calculate the correction coefficients:

   :math:`k_i = \frac{V_i^{SF} + V_i^{NSF}}{<V>}`

   where *SF* and *NSF* correspond to spin-slip and non-spin-flip signal, respectively. :math:`<V>` is calculated in a following way:

   :math:`<V> = \frac{1}{N}\sum <V_i^{SF} + V_i^{NSF}>`

   where :math:`<V_i^{SF} + V_i^{NSF}>` is the mean value of the total signal for :math:`i`-th detector calculated using the :ref:`algm-Mean` algorithm, :math:`N` is the number of not masked detectors. Sum is calculated using the :ref:`algm-SumSpectra` algorithm.

   The :ref:`algm-Divide` algorithm is used for this step.

.. note::

        To apply detector efficiency correction to the data, one should divide neutron counts by the coefficients:

                :math:`(I_i)_{corr} = \frac{I_i}{k_i}`


Valid input workspaces
######################

The input workspaces or groups of workspaces have to have the following in order to be valid inputs for this algorithm.

-  The same number of dimensions
-  The same number of spectra
-  The same number of bins
-  The same kind of normalization in the *normalized* sample log.

For the physically meaningful correction it is also important that these workspaces have the same slits size, polarisation, and the neutron wavelength. If some of these parameters are different, algorithm produces warning. If these properties are not specified in the workspace sample logs, no comparison is performed.


Usage
-----

**Example - Calculate coefficients and apply correction to a single run:**

.. code-block:: python

   # Load Vanadium and background data
   curtable = 'currents.txt'

   vana_sf = LoadDNSLegacy('dn134011vana.d_dat', Normalization='duration', CoilCurrentsTable=curtable)
   vana_nsf = LoadDNSLegacy('dn134012vana.d_dat', Normalization='duration', CoilCurrentsTable=curtable)
   bkgr_sf = LoadDNSLegacy('dn134037leer.d_dat', Normalization='duration', CoilCurrentsTable=curtable)
   bkgr_nsf = LoadDNSLegacy('dn134038leer.d_dat', Normalization='duration', CoilCurrentsTable=curtable)

   # Mask 'bad' detectors
   MaskDetectors(vana_nsf, DetectorList=[1])

   # Calculate correction coefficients
   coefs = DNSComputeDetEffCorrCoefs([vana_sf, vana_nsf], [bkgr_sf, bkgr_nsf])

   print("First 3 correction coefficients: ")
   for i in range(3):
        print(round(coefs.readY(i),2))

   print("Is first detector masked? {}".format(coefs.getInstrument().getDetector(1).isMasked()))

   # load sample data
   rawdata = LoadDNSLegacy('oi196012pbi.d_dat', Normalization='duration', CoilCurrentsTable=curtable)

   # apply correction
   corrected_data = rawdata/coefs
   print("First 3 corrected data points")
   for i in range(3):
        print(round(corrected_data.readY(i),2))

Output:

.. code-block:: none

   First 3 correction coefficients:

   0.0

   1.13

   1.26

   Is first detector masked? True

   First 3 corrected data points

   0.0

   287.89

   277.55

.. categories::

.. sourcelink::
