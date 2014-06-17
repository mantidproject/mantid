.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The fluctuation dissipation theorem [1,2] relates the dynamic
susceptibility to the scattering function

:math:`\left(1-e^{-\frac{E}{k_B T}}\right) S(\mathbf{q}, E) = \frac{1}{\pi} \chi'' (\mathbf{q}, E)`

where :math:`E` is the energy transfer to the system. The algorithm
assumes that the y axis of the input workspace contains the scattering
function :math:`S`. The y axis of the output workspace will contain the
dynamic susceptibility. The temperature is extracted from a log attached
to the workspace, as the mean value. Alternatively, the temperature can
be directly specified. The algorithm will fail if neither option is
valid.

[1] S. W. Lovesey - Theory of Neutron Scattering from Condensed Matter,
vol 1

[2] I. A. Zaliznyak and S. H. Lee - Magnetic Neutron Scattering in
"Modern techniques for characterizing magnetic materials"

.. categories::
