.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is intended to calculate pole figure from engineering material.


Pole Figure
###########

In engineering material science, material is measured from different laboratory coodinates.
It is necessary to transform the laboratory coordiante to sample coordinate.

Pole figure plots a specific peak's intensity in the sample coordinate.


Conversion from lab coordinates to sample coordinates
=====================================================

Note that a :math:`\tilde{x}` denotes a unit vector.

Define :math:`\vec{Q}`
++++++++++++++++++++++

For an arbitrary detector, assuming its position is at :math:`\vec{X} = (x, y, z)`

As the the position of source at :math:`\vec{M} = (0, 0, L_1)` and
that of sample at :math:`\vec{S} = (0, 0, 0)`,

* :math:`\vec{\tilde{k}_i} = |\vec{S} - \vec{M}|`
* :math:`\vec{\tilde{k}_{\tau}} = |\vec{X} - \vec{S}|`

And thus the diffraction vector is defined as :math:`\vec{Q} = \tilde{k}_i - \tilde{k}_{\tau}`

From sample log to rotation angle
+++++++++++++++++++++++++++++++++

* :math:`\Omega ' = \Omega - \psi + 135`
* :math:`\tau " = -HROT - \phi`

The sample logs can be read from sample logs with this mapping

* :math:`\Omega` is from BL7:... 
* *HROT* is from BL7: ... 


First rotation
++++++++++++++

The first rotation is about the Y-axis.

:math:`\tilde{Q}\prime = \tilde{Q} * \cos(\Omega\prime) + (\vec{k}_1 \times \tilde{Q}\sin(\Omega\prime) + \vec{k}_1 (\vec{k_1}\cdot\tilde{Q})(1-\cos(\Omega\prime))`

where :math:`\vec{k}_1 = (0, 1, 0)`

Second rotation
+++++++++++++++

The second rotation is about the Z-axis.

:math:`\tilde{Q}'' = \tilde{Q}'\cos(\tau'') + (\vec{k}_2\times \tilde{Q}')\sin(\tau'') + \vec{k}_2(\vec{k}_2\cdot\tilde{Q}')(1-\cos(\tau''))`

where :math:`\vec{k}_2 = (0, 0, 1)`

Projection to the pole figure
+++++++++++++++++++++++++++++

The coordinates of pole figure are described by :math:`(R_{TD}'', R_{ND}'')`

Now project the twice rotated :math:`Q` to pole figure plane

* :math:`R_{TD}'' = \tilde{Q}_{y}'' \cdot S(\tilde{Q}_z'')`
* :math:`R_{ND}'' = -\tilde{Q}_{x}'' \cdot S(\tilde{Q}_z'')`

where 

* :math:`S(x) = 1`, when :math:`x \leq 0`
* :math:`S(x) = -1`, when :math:`x < 0`


Outputs
=======

Outputs are designed in order to make it convenient for plotting tool, such as matplotlib, to plot the pole figure outside MantidPlot.



.. categories::

.. sourcelink::
