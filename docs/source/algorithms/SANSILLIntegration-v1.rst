.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms performs integration of corrected SANS data.
The input workspace must be processed by :ref:`SANSILLReduction <algm-SANSILLReduction>` as **Sample**.
That is, it has to have instrument associated with it, and be in units of Wavelength.
The output can be chosen among :math:`I(Q)` (default), :math:`I(Phi,Q)` or :math:`I(Qx,Qy)`.
Separate outputs can be generated for each of the detector components, if so desired.

The resolution can be calculated either by following [Mildner-Carpenter]_ approach or using fitted horizontal
size of the direct beam, when the property `CalculateResolution` is set to `DirectBeam`. The latter is currently
supported only for D11, D11lr, and D11B.

OutputBinning
-------------

By default, a sensible Q-binning is calculated based on the size (more precisely, the height) of the pixels in the detector.
If requested, it can be calculated also such that at each :math:`Q` :math:`dQ` is about twice the :math:`Q` resolution.
The latter is calculated following [Mildner-Carpenter]_ approach.

Binning can also be specified manually:

One entry
#########

If one entry is given, it will denote the bin width and the :math:`Q_min` and :math:`Q_max` will be calculated from the instrument configuration.
The **-** can be given to request logarithmic binning.

.. code-block:: python

  OutputBinning=0.1 # equidistant binning of 0.1 AA, range will be computed automatically
  OutputBinning=-0.1 # logarithmic binning of 10%, range will be computed automatically

Two entries
###########

If two entries are given, they will be regarded as user specified :math:`Q_min` and :math:`Q_max`, and the default bin width will be suggested.

.. code-block:: python

  OutputBinning=[0.01,2.5] # default binning within 0.01 AA and 2.5 AA

Three entries
#############

It three entries are given, it will signify the :math:`Q_min`, bin width and :math:`Q_max`. Again **_** sign in the bin width will denote logarithmic binning.

.. code-block:: python

  OutputBinning=[0.01,0.1,2.5] # bins with width of 0.1 within range 0.01-2.5 AA
  OutputBinning=[0.01,-0.1,2.5] # 10% logarithmic binning within range 0.01-2.5 AA

Custom binning
##############

If more than three entries are given (odd number of entries), a fully custom array will be constructed as explained in **Params** property of :ref:`Rebin <algm-Rebin>`.

Usage
-----

Check the example in :ref:`SANSILLReduction <algm-SANSILLReduction>`.

References
----------

.. [Mildner-Carpenter] `J. Appl. Cryst. (1984). 17, 249-256 <https://doi.org/10.1107/S0021889884011468>`_

.. categories::

.. sourcelink::
