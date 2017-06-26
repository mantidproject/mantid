.. _Lattice:

Lattice
=======

The purpose of this document is to explain how Mantid is using
information about unit cells and their orientation with respect to the
laboratory frame. For a detailed description, see the
`UB matrix implementation notes <http://github.com/mantidproject/documents/blob/master/Design/UBMatriximplementationnotes.pdf>`__.

Theory
------

The physics of a system studied by neutron scattering is described by
the conservation of energy and momentum. In the laboratory frame:

.. math::
    
    Q_l=  \hbar \mathbf{k}_i^{} -  \hbar \mathbf{k}_f


.. math::
    
    \Delta E_l= \frac{\hbar^2}{2m} (k_i^2 -  k_f^2)

Note that the left side in the above equations refer to what is
happening to the lattice, not to the neutron.

Let's assume that we have a periodic lattice, described by lattice
parameters :math:`a,\ b,\ c,\ \alpha,\ \beta,\ \gamma`. The reciprocal
lattice will be described by parameters
:math:`a^*,\ b^*,\ c^*,\ \alpha^*,\ \beta^*,\  \gamma^*`. Note that
Mantid uses :math:`a^*=\frac{1}{a}` type of notation, like in
crystallography.

For such a lattice, the physics will be described in terms of reciprocal
lattice parameters by

.. math::
    
    Q= 2 \pi\left(h \mathbf{a}^* + k \mathbf{b}^* +l \mathbf{c}^* \right) = \left(\begin{array}{c}
                                                            h \\
                                                            k \\
                                                            l
                                                          \end{array}\right)

The :math:`UB_{}^{}` matrix formalism relates :math:`Q_l^{}` and
:math:`Q_{}^{}` with the following equation:


.. math::

    Q_l = 2 \pi R \cdot U \cdot B \left(\begin{array}{c}
                                                            h \\
                                                            k \\
                                                            l
                                                          \end{array}\right)

The :math:`B_{}^{}` matrix transforms the :math:`h^{}_{}, k, l` triplet
into a Cartesian system, with the first axis along
:math:`\ \mathbf{a}^*`, the second in the plane defined by
:math:`\ \mathbf{a}^*` and :math:`\ \mathbf{b}^*`, and the third axis
perpendicular to this plane. In the Busing and Levi convention (W. R.
Busing and H. A. Levy, Angle calculations for 3- and 4-circle X-ray and
neutron diffractometers - Acta Cryst. (1967). 22, 457-464):

.. math::
    
    B = \left( \begin{array}{ccc}
        a^* & b^*\cos(\gamma^*) & c^*\cos(\beta^*) \\
        0 & b^*\sin(\gamma^*) & -c^*\sin(\beta^*)\cos(\alpha) \\
        0 & 0 & 1/c \end{array} \right)

The :math:`U_{}^{}` matrix represents the rotation from this Cartesian
coordinate frame to the Cartesian coordinate frame attached to the
innermost axis of the goniometer that holds the sample.

The :math:`R_{}^{}` matrix is the rotation matrix of the goniometer

Other useful equations:


.. math::
    G^* = (UB)^T UB = B^T B = \left( \begin{array}{ccc}
        a^*a^* & a^*b^*\cos(\gamma^*) & a^*c^*\cos(\beta^*) \\
        a^*b^*\cos(\gamma^*) & b^*b^* & b^*c^*\cos(\alpha^*) \\
        a^*c^*\cos(\beta^*) & b^*c^*\cos(\alpha^*) & c^*c^* \end{array} \right)


.. math::
    G=(G^*)^{-1}=\left( \begin{array}{ccc}
        aa & ab\cos(\gamma) & ac\cos(\beta) \\
        ab\cos(\gamma) & bb & bc\cos(\alpha) \\
        ac\cos(\beta) & bc\cos(\alpha) & cc \end{array} \right)

The distance in reciprocal space to the :math:`\left(h,k,l\right)` plane
is given by 

.. math::
    
    d^* =\left| B \left(\begin{array}{c}
                                                            h \\
                                                            k \\
                                                            l
                                                          \end{array}\right)\right|

The distance in real space to the :math:`\left(h,k,l\right)` plane is
given by :math:`d=\frac{1}{d^*}`

The angle between :math:`Q_1^{}` and :math:`Q_2^{}` is given by
:math:`\cos( Q_1^{}, Q_2^{})=\frac{(BQ_1)(BQ_2)}{|(BQ_1)| |(BQ_2)|}`

Unit cells
----------

The :class:`~mantid.geometry.UnitCell` class provides the functions to access direct and
reciprocal lattices. 

Oriented lattices
-----------------

All the functions defined for :class:`~mantid.geometry.UnitCell` are inherited by the
:class:`~mantid.geometry.OrientedLattice` objects. In addition, functions for manipulating
the :math:`U` and :math:`UB` matricies are also provided.

Note about orientation
----------------------

Most of the instruments have incident beam along the :math:`\mathbf{z}`
direction. For an orthogonal lattice with :math:`\mathbf{a}^*` along
:math:`\mathbf{z}`, :math:`\mathbf{b}^*` along :math:`\mathbf{x}`, and
:math:`\mathbf{c}^*` along :math:`\mathbf{y}`, the :math:`U^{}_{}`
matrix has the form:

.. math::
    
    U =  \left( \begin{array}{ccc}
        0 & 1 & 0 \\
        0 & 0 & 1 \\
        1 & 0 & 0 \end{array} \right)



.. categories:: Concepts
