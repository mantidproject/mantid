.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Sassena application `1 <http://sassena.org>`__ generates
intermediate scattering factors from molecular dynamics trajectories.
This algorithm reads Sassena output and stores all data in workspaces of
type `Workspace2D <Workspace2D>`__, grouped under a single
`WorkspaceGroup <WorkspaceGroup>`__. It is implied that the time unit is
one **picosecond**.

Sassena ouput files are in HDF5 format
`2 <http://www.hdfgroup.org/HDF5>`__, and can be made up of the
following datasets: *qvectors*, *fq*, *fq0*, *fq2*, and *fqt*

The group workspace should contain workspaces **\_fqt.Re** and
**\_fqt.Im** containing the real and imaginary parts of the intermediate
structure factor, respectively. This algorithm will take both and
perform :ref:`algm-FFT`, storing the real part of the transform in
workspace **\_fqw** and placing this workspace under the input group
workspace. Assuming the time unit to be one picosecond, the resulting
energies will be in units of one **micro-eV**.

The Schofield correction (P. Schofield, *Phys. Rev. Letters* **4**\ (5),
239 (1960)) is optionally applied to the resulting dynamic structure
factor to reinstate the detailed balance condition
:math:`S(Q,\omega)=e^{\beta \hbar \omega}S(-Q,-\omega)`.

Details
-------

Parameter FFTonlyRealPart
#########################

Setting parameter FFTonlyRealPart to true will produce a transform on
only the real part of I(Q,t). This is convenient if we know that I(Q,t)
should be real but a residual imaginary part was left in a Sassena
calculation due to finite orientational average in Q-space.

Below are plots after application of SassenaFFT to
:math:`I(Q,t) = e^{-t^2/(2\sigma^2)} + i\cdot t \cdot e^{-t^2/(2\sigma^2)}`
with :math:`\sigma=1ps`. Real an imaginary parts are shown in panels (a)
and (b). Note that :math:`I(Q,t)*=I(Q,-t)`. If only :math:`Re[I(Q,t)]`
is transformed, the result is another Gaussian:
:math:`\sqrt{2\pi}\cdot e^{-E^2/(2\sigma'^2)}` with
:math:`\sigma'=4,136/(2\pi \sigma)` in units of :math:`\mu`\ eV (panel
(c)). If I(Q,t) is transformed, the result is a modulated Gaussian:
:math:`(1+\sigma' E)\sqrt{2\pi}\cdot e^{-E^2/(2\sigma'^2)}`\ (panel
(d)).

.. figure:: /images/SassenaFFTexample.jpg
   :alt: SassenaFFTexample.jpg

   SassenaFFTexample.jpg

.. categories::
