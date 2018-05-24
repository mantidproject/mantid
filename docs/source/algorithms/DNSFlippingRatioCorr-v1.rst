.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm applies flipping ratio correction to a given data workspaces. As a result, following workspaces will be created: 

-  output workspace with corrected spin-flip data. Sample logs will be copied from the data spin-flip workspace. 
-  output workspace with corrected non-spin-flip data. Sample logs will be copied from the data non-spin-flip workspace. 

Flipping ratio correction is performed using the measurements of :math:`Ni_{0.89}\,Cr_{0.11}` standard sample (hereafter NiCr). Background for NiCr must be also measured and provided to the algorithm as an input. Both, spin-flip anf non-spin-flip measurements are required. This algorithm performs the flipping ratio correction according to J. Appl. Cryst. 42, 69-84, 2009. Calculations are made in a following steps:

1. Subtract Background from NiCr:

   :math:`N^{SF,\,NSF}_i = (N^{SF,\,NSF}_i)_{raw} - (B^{SF,\,NSF}_i)`

   The :ref:`algm-Minus` algorithm is used for this step. In the case of negative result, the error message will be produced and the algorithm terminates.

2. Calculate the correction coefficients:

   :math:`k_i = \frac{N^{NSF}_i}{N^{SF}_i} - 1`

   The :ref:`algm-Divide` algorithm is used for this step.

3. Apply correction to the data:

   :math:`(I^{NSF}_i)_{corr} = I^{NSF}_i + \frac{1}{k_i}\cdot\left(I^{NSF}_i - I^{SF}_i\right)`
   
   :math:`(I^{SF}_i)_{corr} = I^{SF}_i - \frac{1}{k_i}\cdot\left(I^{NSF}_i - I^{SF}_i\right)`

   where :math:`I^{SF,\,NSF}_i` are the neutron counts in the **SFDataWorkspace** and **NSFDataWorkspace**, respectively.


Valid input workspaces
######################

The input workspaces have to have the following in order to be valid inputs for this algorithm.

-  The same number of dimensions
-  The same number of spectra
-  The same number of bins
-  All given workspaces must have the same polarisation (algorithm checks for the 'polarisation' and 'polarisation_comment' sample logs)
-  All given workspaces must be normalized either to monitor counts or to experiment duration
-  All given workspaces must have the same kind of normalization (algorithm checks for 'normalized' sample log)
-  All given workspaces must have the appropriate flipper status (algorithm checks for 'flipper' sample log): spin-flip workspaces must have flipper 'ON' and non-spin-flip workspaces must have flipper 'OFF'

If any of these conditions is not fulfilled, the algorithm terminates.

For the physically meaningful correction it is also important that these workspaces have the same slits size, detector bank rotation angle and the neutron wavelength. If some of these parameters are different, algorithm produces warning. If these properties are not specified in the workspace sample logs, no comparison is performed.


Usage
-----

**Example - Apply flipping ratio correction to a Vanadium run:**

.. code-block:: python

    from os.path import join
    import numpy as np

    datapath = "/path/to/data/dns/rc36b_standard_dz"
    coilcurrents = join(datapath, 'currents.txt')

    # define input files.
    sf_vanafile = join(datapath, 'dz29100525vana.d_dat')
    nsf_vanafile = join(datapath, 'dz29100526vana.d_dat')

    sf_bkgrfile = join(datapath, 'dz29100645leer.d_dat')
    nsf_bkgrfile = join(datapath, 'dz29100646leer.d_dat')

    sf_nicrfile = join(datapath, 'dz29100585nicr.d_dat')
    nsf_nicrfile = join(datapath, 'dz29100586nicr.d_dat')

    # load files to workspaces
    sf_vana = LoadDNSLegacy(sf_vanafile, Normalization='duration', CoilCurrentsTable=coilcurrents)
    nsf_vana = LoadDNSLegacy(nsf_vanafile, Normalization='duration', CoilCurrentsTable=coilcurrents)

    sf_nicr = LoadDNSLegacy(sf_nicrfile, Normalization='duration', CoilCurrentsTable=coilcurrents)
    nsf_nicr = LoadDNSLegacy(nsf_nicrfile, Normalization='duration', CoilCurrentsTable=coilcurrents)

    sf_bkgr = LoadDNSLegacy(sf_bkgrfile, Normalization='duration', CoilCurrentsTable=coilcurrents)
    nsf_bkgr = LoadDNSLegacy(nsf_bkgrfile, Normalization='duration', CoilCurrentsTable=coilcurrents)

    # for a physically meaningful correction, we must subtract background from Vanadium
    # this step is usually not required for other kinds of samples
    sf_vana_bg = sf_vana - sf_bkgr
    nsf_vana_bg = nsf_vana - nsf_bkgr

    # apply correction
    DNSFlippingRatioCorr(sf_vana_bg, nsf_vana_bg, sf_nicr, nsf_nicr, sf_bkgr, nsf_bkgr,
                         SFOutputWorkspace='sf_corrected', NSFOutputWorkspace='nsf_corrected')

    # retrieve output workspaces
    sf_corrected = mtd['sf_corrected']
    nsf_corrected = mtd['nsf_corrected']

    # calculate ratio of spin-flip to non-spin-flip
    vana_ratio = sf_corrected/nsf_corrected

    # ratio must be around 2, print first 5 points of the data array
    print(np.around(vana_ratio.extractY()[:5]))

Output:

   [[ 2.]
   [ 2.]
   [ 2.]
   [ 2.]
   [ 2.]]

.. categories::

.. sourcelink::
