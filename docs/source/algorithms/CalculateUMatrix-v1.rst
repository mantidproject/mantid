.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a set of peaks (Q in the goniometer frame, HKL values), and given
lattice parameters :math:`(a,b,c,\alpha,\beta,\gamma)`, it will try to
find the U matrix, using least squares approach and quaternions
`1 <http://www.cs.iastate.edu/~cs577/handouts/quaternion.pdf>`__. Units
of length are in in :math:`\rm \AA`, angles are in degrees.

The algorithm calculates first the B matrix according to Busing and
Levi.

Given a set of peaks in the reference frame of the inner axis of the
goniometer, :math:`\rm Q_{gon}`, indexed by :math:`(h_i, k_i, l_i)`, we
want to find the U matrix that maps peaks in the reciprocal space of the
sample to the peaks in the goniometer frame

.. math::
   :label:

   \rm U \rm B \left(
                               \begin{array}{c}
                                 h_i \\
                                 k_i \\
                                 l_i \\
                               \end{array}
                             \right) = \rm Q_{gon,i}

For simplicity, we define

.. math::
   :label:

   \rm Q_{hkl,i} = \rm B \left(
                               \begin{array}{c}
                                 h_i \\
                                 k_i \\
                                 l_i \\
                               \end{array}
                             \right)

In the real world, such a matrix is not always possible to find.
Therefore we just try minimize the difference between the two sets of p

.. math::
   :label: pdiff

   \sum_i |\rm U \rm Q_{hkl,i} - \rm Q_{gon,i}|^2 = \sum_i \left(|\rm U \rm Q_{hkl,i}|^2 + |\rm Q_{gon,i}|^2 -2 \rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right)

In equation :eq:`pdiff`, :math:`\left|\rm U \rm Q_{hkl,i}\right|^2 = |\rm Q_{hkl,i}|^2`, so the
first two terms on the left side are U independent. Therefore we want to
maximize

.. math::
   :label: uqdotqsum

   \sum_i \left(\rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right)

We are going to write the scalar product of the vectors in terms of
quaternions `2 <http://en.wikipedia.org/wiki/Quaternion>`__. We define
:math:`q_{hkl,i} = \left(0, Q_{hkl,i}\right)`,
:math:`q_{gon,i} = \left(0, Q_{gon,i}\right)` and the rotation U is
described by quaternion :math:`u = \left(w,x,y,z\right)`

Then equation :eq:`uqdotqsum` will be written as

.. math::
   :label: uqdotq

   \sum_i \left(\rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right) = 0.5 \cdot \left(u q_{hkl,i} u^*\right) q_{gon,i}\ + 0.5 \cdot q_{gon,i} \left(u q_{hkl,i} u^*\right)

We define matrices

.. math::
   :label: quat_h

   H_i= \left(\begin{array}{cccc}
              0 & -q_{hkl,i,x} & -q_{hkl,i,y} & -q_{hkl,i,z} \\
              q_{hkl,i,x} & 0 & q_{hkl,i,z} & -q_{hkl,i,y} \\
              q_{hkl,i,y} & -q_{hkl,i,z} & 0 & q_{hkl,i,x} \\
              q_{hkl,i,z} & q_{hkl,i,y} & -q_{hkl,i,x} & 0
          \end{array} \right)

and

.. math::
   :label: quat_s

   S_i= \left(\begin{array}{cccc}
              0 & -q_{gonl,i,x} & -q_{gon,i,y} & -q_{gon,i,z} \\
              q_{gon,i,x} & 0 & -q_{gon,i,z} & q_{gon,i,y} \\
              q_{gon,i,y} & q_{gon,i,z} & 0 & -q_{gon,i,x} \\
              q_{gon,i,z} & -q_{gon,i,y} & q_{gon,i,x} & 0
         \end{array} \right)

Then, we can rewrite equation :eq:`uqdotq` using
matrices `3 <http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Pairs_of_unit_quaternions_as_rotations_in_4D_space>`_,
`4 <http://www.cs.iastate.edu/~cs577/handouts/quaternion.pdf>`_:

.. math::
   :label:

   \sum_i \left(\rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right) = \left(\begin{array}{cccc}
                                                                  w & x & y & z\end{array} \right)  \sum_i H_i S_i \left(
                               \begin{array}{c}
                                 w \\
                                 x \\
                                 y \\
                                 z
                               \end{array}
                               \right)

The problem of finding :math:`\left(w,x,y,z\right)` that maximizes the
sum can now be rewritten in terms of eigenvectors of
:math:`HS= \sum_i \left(H_i S_i\right)` . Let :math:`\epsilon_j` and
:math:`\nu_j` be the eigenvalues and corresponding eigenvectors of
:math:`HS`, with
:math:`\epsilon_0 > \epsilon_1 > \epsilon_2 > \epsilon_3`. We can write
any vector :math:`(w,x,y,z)` as a linear combination of the eigenvectors
of :math:`HS`:

.. math::
  :label:

   \left(w,x,y,z\right) = \delta_0 \nu_0 +\delta_1 \nu_1 +\delta_2 \nu_2 +\delta_3 \nu_3

.. math::
  :label:

   \left(\begin{array}{cccc}
         w & x & y & z\end{array} \right)  HS \left(
                               \begin{array}{c}
                                 w \\
                                 x \\
                                 y \\
                                 z
                               \end{array}
                             \right) = \delta_0^2 \nu_0 HS \nu_0 + \delta_1^2 \nu_1 HS \nu_1 +\delta_2^2 \nu_2 HS \nu_2 +\delta_3 \nu_3 HS \nu_3

.. math::
  :label: quatsum

  \begin{split}
                                  &  = \delta_0^2 \epsilon_0 + \delta_1^2 \epsilon_1 +\delta_2^2 \epsilon_2 +\delta_3 ^2 \epsilon_3
  \end{split}

where :math:`u` is a unit quaternion,
:math:`\delta_0^2  + \delta_1^2 +\delta_2^2 +\delta_3 ^2=1` (12)

Then the sum in equation :eq:`quatsum` is maximized for :math:`\epsilon_0 =1, \epsilon_1 =0,  \epsilon_2 =0 \epsilon_3 =0`

Therefore U is the rotation represented by the quaternion :math:`u`,
which is the eigenvector corresponding to the largest eigenvalue of
:math:`HS`.

For more information see the documentation for :ref:`UB matrix <Lattice>`.

.. categories::

.. sourcelink::
