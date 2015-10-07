.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm applies flipping ratio correction to a given data workspaces. As a result, following workspaces will be created: 

-  output workspace with corrected spin-flip data. Sample logs will be copied from the data spin-flip workspace. 
-  output workspace with corrected non-spin-flip data. Sample logs will be copied from the data non-spin-flip workspace. 
-  if data workspaces have workspaces with normalization data (monitor counts or experiment duration by user's choice), this normalization workspaces will be cloned. The normalization workspace is named same as the output workspace, but has suffix "_NORM". 

Flipping ratio correction is performed using the measurements of :math:`Ni_{0.89}\,Cr_{0.11}` standard sample (hereafter NiCr). Background for NiCr must be also measured and provided to the algorithm as an input. Both, spin-flip anf non-spin-flip measurements are required. This algorithm performs the flipping ratio correction for a particular run in following steps:

1. Normalize both, spin-flip (hereafter SF) and non-spin-flip (hereafter NSF), workspaces to a chosen normalization:

   :math:`(N^{SF}_i)_{Norm} = \frac{(N^{SF}_i)_{Raw}}{(C^{SF}_i)_N}`

   :math:`(N^{NSF}_i)_{Norm} = \frac{(N^{NSF}_i)_{Raw}}{(C^{NSF}_i)_N}`

   where :math:`(N^{SF,\,NSF}_i)_{Raw}` is the signal from the :math:`i` th detector in the NiCr spin-flip and non-spin-flip workspace, respectively and :math:`(C^{SF,\,NSF}_i)_N` is the number in the corresponding bin of the normalization workspace. The :ref:`algm-Divide` algorithm is used for this step.

2. Normalize Background workspace to a chosen normalization:

   :math:`(B^{SF,\,NSF}_i)_{Norm} = \frac{(B^{SF,\,NSF}_i)_{Raw}}{(C^{SF,\,NSF}_i)_B}`
   
   where :math:`(B^{SF,\,NSF}_i)_{Raw}` is the signal from the :math:`i` th detector in the spin-flip and non-spin-flip background workspace, respectively and :math:`(C^{SF,\,NSF}_i)_B` is the number in the corresponding bin of the normalization workspace. The :ref:`algm-Divide` algorithm is used for this step.

.. warning::

    Normalization workspaces are created by the :ref:`algm-LoadDNSLegacy` algorithm. 
    It is responsibility of the user to take care about the same type of normalization (either monitor counts or run duration) 
    for given workspaces.

3. Subtract Background from NiCr:

   :math:`N^{SF,\,NSF}_i = (N^{SF,\,NSF}_i)_{Norm} - (B^{SF,\,NSF}_i)_{Norm}`

   The :ref:`algm-Minus` algorithm is used for this step. In the case of negative result, the error message will be produced and the algorithm terminates.

4. Calculate the correction coefficients:

   :math:`k_i = \frac{N^{NSF}_i}{N^{SF}_i}`

   The :ref:`algm-Divide` algorithm is used for this step.

5. Apply correction to the data:

   :math:`(I^{NSF}_i)_{corr0} = I^{NSF}_i - \frac{I^{SF}_i}{k_i}`
   
   :math:`(I^{SF}_i)_{corr0} = I^{SF}_i - \frac{I^{NSF}_i}{k_i}`

   where :math:`I^{SF,\,NSF}_i` are the neutron counts in the **SFDataWorkspace** and **NSFDataWorkspace**, respectively.

6. Apply correction for a double spin-flip scattering:

   :math:`(I^{NSF}_i)_{corr} = (I^{NSF}_i)_{corr0} - (I^{SF}_i)_{corr0}\cdot f`

   :math:`(I^{SF}_i)_{corr} = (I^{SF}_i)_{corr0}`

   where :math:`f` is a double spin-flip scattering probability. It is a number between 0 and 1.


Valid input workspaces
######################

The input workspaces have to have the following in order to be valid inputs for this algorithm.

-  The same number of dimensions
-  The same number of spectra
-  The same number of bins
-  All workspaces except of **SFDataWorkspace** and **NSFDataWorkspace** must have the corresponding normalization workspace
-  All given workspaces must have the same polarisation (algorithm checks for the 'polarisation' sample log)
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

    # define input files.
    sf_vanafile = join(datapath, 'dz29100525vana.d_dat')
    nsf_vanafile = join(datapath, 'dz29100526vana.d_dat')

    sf_bkgrfile = join(datapath, 'dz29100645leer.d_dat')
    nsf_bkgrfile = join(datapath, 'dz29100646leer.d_dat')

    sf_nicrfile = join(datapath, 'dz29100585nicr.d_dat')
    nsf_nicrfile = join(datapath, 'dz29100586nicr.d_dat')

    # load files to workspaces
    sf_vana = LoadDNSLegacy(sf_vanafile, Normalization='duration', Polarisation='x')
    nsf_vana = LoadDNSLegacy(nsf_vanafile, Normalization='duration', Polarisation='x')

    sf_nicr = LoadDNSLegacy(sf_nicrfile, Normalization='duration', Polarisation='x')
    nsf_nicr = LoadDNSLegacy(nsf_nicrfile, Normalization='duration', Polarisation='x')

    sf_bkgr = LoadDNSLegacy(sf_bkgrfile, Normalization='duration', Polarisation='x')
    nsf_bkgr = LoadDNSLegacy(nsf_bkgrfile, Normalization='duration', Polarisation='x')

    # for a physically meaningful correction, we must subtract background from Vanadium
    # this step is usually not required for other kinds of samples
    # retrieve normalization workspaces
    sf_vana_norm = mtd['sf_vana_NORM']
    sf_bkgr_norm = mtd['sf_bkgr_NORM']
    nsf_vana_norm = mtd['nsf_vana_NORM']
    nsf_bkgr_norm = mtd['nsf_bkgr_NORM']
    # subtract background
    sf_vana_bg = sf_vana/sf_vana_norm - sf_bkgr/sf_bkgr_norm
    nsf_vana_bg = nsf_vana/nsf_vana_norm - nsf_bkgr/nsf_bkgr_norm

    # apply correction
    DNSFlippingRatioCorr(sf_vana_bg, nsf_vana_bg, sf_nicr, nsf_nicr, sf_bkgr, nsf_bkgr,
                         SFOutputWorkspace='sf_corrected', NSFOutputWorkspace='nsf_corrected',
                         DoubleSpinFlipScatteringProbability=0.03)

    # retrieve output workspaces
    sf_corrected = mtd['sf_corrected']
    nsf_corrected = mtd['nsf_corrected']

    # calculate ratio of spin-flip to non-spin-flip
    vana_ratio = sf_corrected/nsf_corrected

    # ratio must be around 2, print first 5 points of the data array
    print np.around(vana_ratio.extractY()[:5])

Output:

   [[ 2.]
   [ 2.]
   [ 2.]
   [ 2.]
   [ 2.]]

.. categories::

.. sourcelink::
