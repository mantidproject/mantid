.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm applies detector efficiency correction to a given data :ref:`Workspace2D <Workspace2D>`. As a result, output workspace with corrected data will be created. Sample logs will be copied from the data workspace. 

Detector efficiency correction is performed using the measurements of vanadium standard sample (hereafter Vanadium). Background for Vanadium must be also measured and provided to the algorithm as an input **BkgWorkspace**. Vanadium and its background can be measured at several detector bank positions.  This algorithm does the detector efficiency for a particular run in following steps:

.. warning::

    Algorithm requires data normalized either to monitor counts or experiment duration. The normalization must be the same for all input workspaces.

1. Subtract Background from Vanadium:

   :math:`V_i = (V_i)_{raw} - B_i`

   The :ref:`algm-Minus` algorithm is used for this step. In the case of negative result, the error message will be produced and the algorithm terminates.

2. Calculate the correction coefficients:

   :math:`k_i = \frac{<V>}{V_i}`

   where :math:`<V>` is a mean value of Vanadium counts. The :ref:`algm-Divide` algorithm is used for this step.

.. note::
    
    If no **VanadiumMean** workspace is given as an input, the :math:`<V>` will be calculated as :math:`<V> = \frac{1}{n}\sum V_i`. However, in the case if correction is applied to a group of runs, :math:`<V>` must be calculated as for the whole group, saved to **VanadiumMean** workspace and provided to this algorithm as an input.

3. Apply correction to the data:

   :math:`(I_i)_{corr} = k_i\times I_i`

   where :math:`I_i` are the neutron counts in the **InputWorkspace**.


Valid input workspaces
######################

The input workspaces (**InputWorkspace**, **VanaWorkspace**, **BkgWorkspace**) have to have the following in order to be valid inputs for this algorithm.

-  The same number of dimensions
-  The same number of spectra
-  The same number of bins
-  The same kind of normalization in the *normalized* sample log.

For the physically meaningful correction it is also important that these workspaces have the same slits size, polarisation, detector bank rotation angle, flipper status and the neutron wavelength. If some of these parameters are different, algorithm produces warning. If these properties are not specified in the workspace sample logs, no comparison is performed.

The workspace **VanadiumMean** is an optional parameter. However, if it is specified, it must be a :ref:`Workspace2D <Workspace2D>` with 2 dimensions, 1 histogram and 1 bin.


Usage
-----

**Example - Apply correction to a single run:**

.. code-block:: python

   # data, vanadium and background files.
   datafile = 'oi196012pbi.d_dat'
   vanafile = 'dn134011vana.d_dat'
   bkgrfile = 'dn134031leer.d_dat'
   coilcurrents = 'currents.txt'

   # Load datasets, loader will create an additional normalization workspace
   data_ws = LoadDNSLegacy(datafile, CoilCurrentsTable=coilcurrents, Normalization='duration')
   vana_ws = LoadDNSLegacy(vanafile, CoilCurrentsTable=coilcurrents, Normalization='duration')
   bkgr_ws = LoadDNSLegacy(bkgrfile, CoilCurrentsTable=coilcurrents, Normalization='duration')

   corrected = DNSDetEffCorrVana(data_ws, vana_ws, bkgr_ws)

   for i in range(3):
    print round(corrected.readY(i), 2)

Output:

   457.89

   268.78

   262.63

.. categories::

.. sourcelink::
