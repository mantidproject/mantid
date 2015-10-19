.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm creates a workspace with  detector sensitivity correction coefficients using the given Vanadium workspace. The correction coefficients are calculated as follows.

1. Calculate the Debye-Waller factor according to Sears and Shelley *Acta Cryst. A* **47**, 441 (1991):

   :math:`D_i = \exp\left(-B_i\cdot\frac{4\pi\sin\theta_i}{\lambda^2}\right)`

   :math:`B_i = \frac{3\hbar^2\cdot 10^{20}}{2m_VkT_m}\cdot J(y)`

   where :math:`J(y) = 0.5` if :math:`y < 10^{-3}`, otherwise

   :math:`J(y) = \int_0^1 x\cdot\mathrm{coth}\left(\frac{x}{2y}\right)\,\mathrm{d}x`

   where :math:`y=T/T_m` is the ratio of the temperature during the experiment :math:`T` to the Debye temperature :math:`T_m = 389K`, :math:`m_V` is the Vanadium atomic mass (in kg) and :math:`\theta_i` is the polar angle of the i-th detector.

.. warning::

    If sample log *temperature* is not present in the given Vanadium workspace or temperature is set to an invalid value, T=293K will be taken for the Debye-Waller factor calculation. Algorithm will produce warning in this case.

2. Perform Gaussian fit of the data to find out the position of the peak centre and FWHM. These values are used to calculate sum :math:`S_i` as

   :math:`S_i = \sum_{x = x_C - 3\,\mathrm{fwhm}}^{x_C + 3\,\mathrm{fwhm}} Y_i(x)`

   where :math:`x_C` is the peak centre position and :math:`Y_i(x)` is the coresponding to :math:`x` :math:`Y` value for i-th detector.

3. Finally, the correction coefficients :math:`K_i` are calculated as

   :math:`K_i = D_i\times S_i`

Workspace containing these correction coefficients is created as an output and can be used as a RHS workspace in :ref:`algm-Divide` to apply correction to the LHS workspace.

.. note::
    
    If gaussian fit fails, algorithm terminates with an error message. The error message contains name of the workspace and detector number.

Restrictions on the input workspace
###################################

The valid input workspace:

- must have an instrument set
- must have a *wavelength* sample log


Usage
-----

**Example**

.. testcode:: ExComputeCalibrationCoefVan

    # load Vanadium data
    wsVana = LoadMLZ(Filename='TOFTOFTestdata.nxs')
    # calculate correction coefficients
    wsCoefs = ComputeCalibrationCoefVan(wsVana)
    print 'Spectrum 4 of the output workspace is filled with: ', round(wsCoefs.readY(999)[0])

    # wsCoefs can be used as rhs with Divide algorithm to apply correction to the data 
    wsCorr = wsVana/wsCoefs
    print 'Spectrum 4 of the input workspace is filled with: ', round(wsVana.readY(999)[0], 1)
    print 'Spectrum 4 of the corrected workspace is filled with: ', round(wsCorr.readY(999)[0], 5)

Output:    

.. testoutput:: ExComputeCalibrationCoefVan

    Spectrum 4 of the output workspace is filled with:  6596.0
    Spectrum 4 of the input workspace is filled with:  1.0
    Spectrum 4 of the corrected workspace is filled with:  0.00015

.. categories::

.. sourcelink::
