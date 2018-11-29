.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm creates a workspace with detector sensitivity correction coefficients using the given Vanadium workspace. The correction coefficients are calculated as follows.

1. Load the peak centre and sigma from the *EPPTable*. These values are used to calculate sum :math:`S_i` as

   :math:`S_i = \sum_{x = x_C - 3\,\mathrm{fwhm}}^{x_C + 3\,\mathrm{fwhm}} Y_i(x)`

   where :math:`x_C` is the peak centre position and :math:`Y_i(x)` is the corresponding to :math:`x` :math:`Y` value for i-th detector.

2. (If *EnableDWF* is true) Calculate the Debye-Waller factor according to [#SEARS]_:

   :math:`D_i = \exp\left[-B_i\cdot\left(\frac{4\pi\sin\theta_i}{\lambda}\right)^2\right]`

   :math:`B_i = \frac{3\hbar^2\cdot 10^{20}}{2m_VkT_m}\cdot J(y)`

   where :math:`J(y) = 0.5` if :math:`y < 10^{-3}`, otherwise

   :math:`J(y) = \int_0^1 x\cdot\mathrm{coth}\left(\frac{x}{2y}\right)\,\mathrm{d}x`

   where :math:`y=T/T_m` is the ratio of the temperature during the experiment :math:`T` to the Debye temperature :math:`T_m = 389K`, :math:`m_V` is the Vanadium atomic mass (in kg) and :math:`\theta_i` is the polar angle of the i-th detector. By default, the temperature is read from the sample logs. The log entry can be given as the 'temperature_sample_log' in the IPF, otherwise 'temperature' entry is used. If the log is missing, or incorrect, the *Temperature* input property can be used instead.

.. warning::

    If no temperature is available, or is set to an invalid value, :math:`T` = 293K will be taken for the Debye-Waller factor calculation. The algorithm will log a warning in this case.

3. (If *EnableDWF* is true) Finally, the correction coefficients :math:`K_i` are calculated as

   :math:`K_i = \frac{S_i}{D_i}`

Workspace containing these correction coefficients is created as an output and can be used as a RHS workspace in :ref:`algm-Divide` to apply correction to the LHS workspace.



Restrictions on the input workspaces
####################################

The valid input workspace:

- must have an instrument set
- must have a *wavelength* sample log

Restrictions for *EPPTable*:

- number of rows of the table must match to the number of histograms of the input workspace.
- table must have the *PeakCentre* and *Sigma* columns.

.. note::
    The input *EPPTable* can be produced using the :ref:`algm-FindEPP` algorithm.


Usage
-----

.. include:: ../usagedata-note.txt

**Example**

.. testcode:: ExComputeCalibrationCoefVan

    # load Vanadium data
    wsVana = LoadMLZ(Filename='TOFTOFTestdata.nxs')
    # find elastic peak positions
    epptable = FindEPP(wsVana)
    # calculate correction coefficients
    wsCoefs = ComputeCalibrationCoefVan(wsVana, epptable)
    print('Spectrum 4 of the output workspace is filled with:  {}'.format(round(wsCoefs.readY(999)[0])))

    # wsCoefs can be used as rhs with Divide algorithm to apply correction to the data
    wsCorr = wsVana/wsCoefs
    print('Spectrum 4 of the input workspace is filled with:  {}'.format(round(wsVana.readY(999)[0], 1)))
    print('Spectrum 4 of the corrected workspace is filled with:  {}'.format(round(wsCorr.readY(999)[0], 5)))

Output:

.. testoutput:: ExComputeCalibrationCoefVan

    Spectrum 4 of the output workspace is filled with:  6895.0
    Spectrum 4 of the input workspace is filled with:  1.0
    Spectrum 4 of the corrected workspace is filled with:  0.00015

References
----------

.. [#SEARS] Sears, V. F. and Shelley, S. A., *Acta Cryst. A* **47** 441 (1991)
          `doi: 10.1107/S0108767391002441 <http://dx.doi.org/10.1107/S0108767391002441>`_

.. categories::

.. sourcelink::
