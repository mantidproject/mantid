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

Note that a :math:`\tilt{x}` denotes a unit vector.

Define :math:`\vec{Q}`
++++++++++++++++++++++

For an arbitrary detector, assuming its position is at :math:`\vec{X} = (x, y, z)`

As the the position of source at :math:`\vec{M} = (0, 0, L_1)` and
that of sample at :math:`\vec{S} = (0, 0, 0)`,

* :math:`\vec{\tilt{k}_i} = |\vec{S} - \vec{M}|`
* :math:`\vec{\tilt{k}_{\tau} = |\vec{X} - \vec{S}|`

And thus the diffraction vector is defined as :math:`\vec{Q} = \tilt{k}_i - \tilt{k}_{\tau}`

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

:math:`\tilt{Q}' = \tilt{Q} \cos(\Omega ') + (\vec{k}_1 \times \tilt{Q}\sin(\Omega ') + \vec{k}+1 (\vec{k+1\cdot\tilt{Q})(1-\cos(\Omega '))`

where :math:`\vec{k}_1 = (0, 1, 0)`

Second rotation
+++++++++++++++

The second rotation is about the Z-axis.

:math:`\tilt{Q}^{pp} = \tilt{Q}'\cos(\tau^{\pp}) + (\vec{k}_2\times \tilt{Q}')\sin(\tau^{\pp}) + \vec{k}_2(\vec{k}_2\cdot\tilt{Q}')(1-\cos(\tau^{\pp}))`

where :math:`vec{k}_2 = (0, 0, 1)`

Projection to the pole figure
+++++++++++++++++++++++++++++

The coordinates of pole figure are described by :math:`(R_{TD}^{\pp}, R_{ND}^{\pp})`

Now project the twice rotated :math:`Q` to pole figure plane

* :math:`R_{TD}^{\pp} = \titl{Q}_{y}^{\pp} \cdot S(\titl{Q}_z^{\pp})`
* :math:`R_{ND}^{\pp} = -\titl{Q}_{x}^{\pp} \cdot S(\titl{Q}_z^{\pp})`

where 

* :math:`S(x) = 1`, when :math:`x \leq 0`
* :math:`S(x) = -1`, when :math:`x < 0`


Outputs
=======

Outputs are designed in order to make it convenient for plotting tool, such as matplotlib, to plot the pole figure outside MantidPlot.



.. categories::

.. sourcelink::
