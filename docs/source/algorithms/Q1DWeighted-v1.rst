.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs azimuthal averaging for a 2D SANS data set by going through
each detector pixel, determining its Q-value, and adding its amplitude
:math:`I` to the appropriate Q bin. For greater precision, each detector
pixel can be sub-divided in sub-pixels by setting the ``NPixelDivision``
parameters. Each pixel has a weight of 1 by default, but the weight of
each pixel can be set to :math:`1/\Delta I^2` by setting the
``ErrorWeighting`` parameter to True.

See the :ref:`Rebin <algm-Rebin>` documentation for details about choosing the ``OutputBinning`` parameter.

See `SANSReduction <http://www.mantidproject.org/Reduction_for_HFIR_SANS>`__
documentation for calculation details.

Wedges
------

For unisotropic scatterer, the *I(Q)* can be calculated for different angular sectors (or wedges).
This can be done in two ways: symmetric wedges (default) or asymmetric wedges.

Symmetric
#########

The figure below illustrates an example for symmetric wedges. Each wedge in this case represents two back-to-back sectors.
The wedges output group will have two workspaces: one for the red region, one for the blue region.

.. figure:: /images/wedge_symm.png
  :align: center
  :width: 600

Asymmetric
##########

An example for asymmetric wedges is shown below. The output will have four workspaces, one per each sector of different color.

.. figure:: /images/wedge_asymm.png
  :align: center
  :width: 600

Usage
-----

This algorithm is not intended to be run individually, rather as a part of the `SANSReduction <http://www.mantidproject.org/Reduction_for_HFIR_SANS>`_.

.. categories::

.. sourcelink::
