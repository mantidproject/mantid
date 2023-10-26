.. _Crystal Field Theory:

Crystal Field Theory
====================

.. contents:: Table of Contents
  :local:


Introduction
------------

This page contains the theory and background to the crystal field calculations.
If you want to know how to make these calculations please see the :ref:`Python interface page <Crystal Field Python Interface>`
or the :ref:`examples page <Crystal_Field_Examples>`.


The Crystal Field Potential
---------------------------

The crystal field (or in older terminology, *crystalline electric field*)
is the potential acting on a particular magnetic ion :math:`i` at position :math:`\mathbf{r}_i` in a solid or molecule.
When first proposed by Van Vleck, it was thought to be an electrostatic effect due to the charges
:math:`q_j` of neighbouring ions, located at :math:`\mathbf{r}_j`, of the magnetic ion (hence the original name).
This leads to an approximation in which the surrounding ions are treated as point charges,
so that the electric potential acting on the magnetic ion is

.. math::
    V_{\mathrm{CF}}(\mathbf{r}_i) = \frac{1}{4\pi\epsilon_0} \sum_j \frac{q_j}{|\mathbf{r}_j - \mathbf{r}_i|}.
  :label: cfpot

This potential satisfies Laplace's equation, :math:`\nabla^2 V(\mathbf{r}) = 0`,
and so can be expanded in terms of spherical harmonic functions

.. math::
    V_{\mathrm{CF}}(\mathbf{r}_i) = \sum_{lm} A_l^m r^l Y_{lm}(\mathbf{r}_i),
  :label: cfexp

where the radial part is

.. math::
    A_l^m = \frac{1}{4\pi\epsilon_0} \left\{ (-1)^m \frac{4\pi}{2l+1} \sum_j \frac{q_j}{r_j^{l+1}} Y_{l,-m}(\mathbf{r}_j) \right\}.
  :label: alm

The above equations define the *point charge model* `[Hutchings64]`_.
In many solids, though, the point charge approximation breaks down,
because the ligand electrons may be involved in bonding or charge transfer processes,
and thus have a spatial extent and may no longer be treated as point charges situated at the atomic sites.
The formulism detailed above, however, was still found to be applicable to a wide variety of materials,
*if* the radial part :math:`A_l^m r^l` is treated as a variable *crystal field parameter*.
This is because the spherical harmonic functions form a complete orthogonal basis set,
so that any function may be expressed as an expansion in them.
Values obtained from equation :eq:`alm`, however, do not generally match experimentally determined crystal field parameters,
except for some insulators and if the point charges :math:`q_j` are scaled from their nominal valences.

Mantid contains routines which can calculate the :math:`A_l^m` parameters
which may be used as starting parameters for a fit to inelastic neutron scattering data.


Crystal Field Energy Levels
---------------------------

The above potential, being an electric interaction, acts only on the orbital part of the electronic wavefunctions.
Its effect is to lift the degeneracy of the orbital states
(labelled by the secondary orbital angular momentum quantum number :math:`L_z`)
of the magnetic ion :math:`i` giving rise to a set of energy levels.

At zero temperature only the ground state energy level is occupied by the electrons of the magnetic ion
but as the temperature increases, higher energy excited levels will become occupied.
The different levels have different magnetic moment expectation values,
which leads to a change of the magnetic susceptibility of materials containing this magnetic ion with temperature
as the population of the levels change.
Because different magnetic materials have different crystal structures and hence a different local crystalline
environment around the magnetic ions, they will also have different susceptibilities,
which is determined in part by the crystal field.
An applied magnetic field will change the energies of the levels with respect to each other (the Zeeman splitting)
so the crystal field will also affect the magnetisation (the magnetic moment as a function of applied magnetic field).

Inelastic neutron spectroscopy can measure the energy difference between these crystal field energy levels
as neutrons may excite or de-excite electrons occupying one level to another.
Finally, the spin-orbit interaction also means that the preferred spin orientation is coupled to the
orbital state and hence is affected by the crystal field, leading in some cases to a preferred ("easy") direction.

In the rare earth ions, the spin-orbit interaction is strong enough in mixing the spin and orbital angular momentum
states that they cannot be labelled by the total spin :math:`S` or orbital :math:`L` quantum numbers.
Instead the total angular momentum quantum number :math:`J` and secondary :math:`J_z` is used to label these states.
The crystal field is now taken to lift the degeneracy of the :math:`J_z` states.
Usually only the ground state :math:`J` multiplet is included in the calculation
because the energy to the next highest :math:`J` level is much higher than room temperature
or the usual energy range of an inelastic neutron scattering experiment.

The Crystal Field Hamiltonian
-----------------------------

In order to calculate the energy splitting of the :math:`J_z` states due to the crystal field,
we define a crystal field Hamiltonian matrix using the :math:`J_z` states as basis states.
The eigenvalues of this Hamiltonian matrix are then the energy levels.
The Hamiltonian is constructed just like the potential in equation :eq:`cfexp`,
but rather than being a sum of the spherical harmonic functions acting on the position coordinate,
it must be a sum over tensor operators which act on the :math:`J_z` basis states.

These operators could be *spherical tensor operators* :math:`T_q^{(k)}`
which transform in the same way under rotations as the spherical harmonic functions,
or they could be hermitian combinations of these operators
which transform in the same way as the *tesseral harmonic functions* (also called real spherical harmonic functions)
:math:`Z_{lm}`:

.. math::
    Z_{lm} = \left\{ \begin{array}{ll}
            \frac{i}{\sqrt{2}} \left[ Y_{lm} - (-1)^m Y_{l,-m} \right] & m<0 \\
                                      Y_{lm} & m=0 \\
            \frac{1}{\sqrt{2}} \left[ Y_{l,-m} + (-1)^m Y_{lm} \right] & m>0 \\
    \end{array} \right.,
  :label: zlm

Expressing the Hamiltonian in terms of the hermitian operators means that the coefficients in the sum can be purely real
(using spherical tensor operators means the coefficients are in general complex) [#]_.

The first attempt to construct such a crystal field Hamiltonian was by Stevens `[Stevens52]`_ who took the expressions
for the tesseral harmonic functions  in `Cartesian coordinates <http://www.mcphase.de/manual/node123.html>`_,
removed the constant prefactors and replaced the :math:`x`, :math:`y` and :math:`z` coordinates
with the angular momentum operators :math:`\hat{J}_x`, :math:`\hat{J}_y`, :math:`\hat{J}_z` respectively,
taking care to obey the commutation relations of the angular momentum operators.
These `Stevens operators <http://www.mcphase.de/manual/node124.html>`_ are used in Mantid and are calculated
from the :math:`\hat{J}_x`, :math:`\hat{J}_y`, :math:`\hat{J}_z` operators expressed as a matrix
using the :math:`J_z` basis states.

In Stevens' original work `[Stevens52]`_, attention was paid to how the crystal field parameters determined for one magnetic
ion might be transfered to another ion in the same crystalline environment.
In order to account for the different electronic configurations of the ions,
the crystal field parameters are additionally weighted by the *Stevens factor*
:math:`\theta_k = \langle \nu,L,S | |O^{(k)}|| \nu,L,S \rangle` which may be thought of as an additional reduced matrix
element which depends on quantum numbers :math:`\nu` other than the angular momentum quantum numbers.
The values of :math:`\theta_k` are tabulated in `Table 1`_,
or may be calculated using the techniques in `[Judd63]`_.
Thus Stevens' Hamiltonian is

.. math::
    \mathcal{H}_{\mathrm{CEF}}^{\mathrm{Stevens}} = \sum_{k=0,2,4,6} \sum_{q=-k}^k A_q^k \langle r^k\rangle \theta_k O_q^{(k)},
  :label: stevcf

where :math:`O_q^{(k)}` are the Stevens operator described above.
In principle, the parameter :math:`A_q^k` is intrinsic to a particular crystalline environment,
whilst the :math:`\langle r^k \rangle \theta_k` parts depend on the magnetic ion within that environment and may be factored out.
In practice, however, it was found that although in some cases the parameters may be transfered between ions, this often fails.

Instead, in the neutron spectroscopy literature, the full product

.. math::
    B_q^k = A_q^k \langle r^k\rangle \theta_k
  :label: bkq

is often used as the crystal field parameter to be fitted, and this convention is used by Mantid
(e.g. the fittable coefficients in Mantid are the :math:`B_q^k` rather than the :math:`A_q^k`).

An alternative formulation of the crystal field Hamiltonian developed by Wybourne `[Wybourne65]`_, Judd `[Judd63]`_ and others used
the spherical tensor operators :math:`T_q^{(k)}` instead of the Stevens operators.
The matrix elements of the :math:`T_q^{(k)}` are then calculated directly using the Wigner-Eckart theorem.
This is a faster calculation but results in different Hamiltonian matrix elements for a given set of crystal field parameter values.
There is therefore a different "normalisation" of crystal field parameters depending on the formulism used to define the operators in the Hamiltonian.
The "Stevens normalisation" is commonly used in neutron scattering and by physicists and is used in Mantid.
The "Wybourne normalisation" is commonly used in the optical spectroscopy community and in chemistry.
The difference between the two are discussed in more detail in the appendix__.

__ `Appendix A: Wybourne Normalisation`_


External and Molecular Magnetic Field Contributions
---------------------------------------------------

In addition to the crystal field Hamiltonian, there are two additional terms that contribute to the total Hamiltonian, the Hamiltonians for the external and molecular magnetic fields:

.. math::
  & H = H_{\mathrm{CEF}} + H_{\mathrm{EXT}} + H_{\mathrm{MOL}} \\

The external field Hamiltonian describes the splitting of electronic energy levels of a magnetic ion in solids due to the presence of a magnetic field.
The molecular field Hamiltonian describes the "internal" fields generated by the ordered moments of neighboring ions.
By convention, the external magnetic fields are separated from the 'internal' magnetic field, which is also called a "molecular" field in older literature.
Both of these effects can be accounted for in Mantid's calculation.
The external applied field is known by the user, whilst the internal "molecular" field is (usually) unknown and needs to be fitted to measured data.

In Mantid we define these two fields as:

.. math::
  & H_{\mathrm{EXT}} = g_J \mu_B \mathbf{J}\cdot\mathbf{B}_{\mathrm{EXT}} \\
  & H_{\mathrm{MOL}} = 2 (g_J - 1) \mu_B \mathbf{J}\cdot\mathbf{B}_{\mathrm{MOL}}

where :math:`g_J` is the Lande g-factor, :math:`\mu_B` is the Bohr magneton, :math:`\mathbf{B}` is the magnetic field vector (in Tesla in Mantid) and :math:`\mathbf{J}` is the total angular momentum operator vector.
This definition follows the `FOCUS manual <https://epubs.stfc.ac.uk/manifestation/5723/RAL-TR-95-023.pdf>`_.


Symmetry considerations
-----------------------

The crystal field potential must be invariant under the operations of the point group of the atomic site at which the magnetic ion is positioned.
The point groups are defined by reflections, rotations, roto-inversions, and inversion.
The multipolar expansion, equation :eq:`cfexp`, means that we just need to determine
which spherical harmonics terms :math:`Y_{lm}` are invariant under each of these operations.
:math:`Y_{lm}` terms which are not invariant under the operations of the point group of the magnetic ion must thus be zero.

To determine the non-zero :math:`Y_{lm}` terms we need only consider the highest symmetry operation, since this is the most restrictive.
Taking the principle axis as :math:`z`, the :math:`n`-fold rotations :math:`C_n` will change :math:`\phi` by :math:`2\pi/n`.
Now, :math:`Y_{l,\pm m} \propto \exp(\pm i m \phi)`.
Thus only terms with :math:`m` being an integer multiple of :math:`n` will be non-zero.
A mirror plane perpendicular to :math:`x` will map :math:`x` to :math:`-x`, or :math:`\cos\phi\rightarrow-\cos\phi=\cos(\phi+\pi)`
and so is equivalent to a rotation by :math:`180^\circ`.
Thus any point group with a mirror plane must have only even :math:`m` terms,
because we can always choose the crystal field coordinate system such that the mirror plane is the :math:`yz` plane
(if there are no other symmetries in the system).

The roto-inversion operation :math:`S_n` is an :math:`n`-fold rotation followed by mirroring in the plane perpendicular to the rotation axis.
This mirroring is actually equivalent to an inversion about the origin,
mapping :math:`x` to :math:`-x`, :math:`y` to :math:`-y` and :math:`z` to :math:`-z`.
In spherical coordinates this means :math:`\cos\theta\rightarrow-\cos\theta=` and :math:`\sin\theta\rightarrow-\sin\theta`.
Thus the presence of a centre of inversion implies that only terms with even powers of :math:`\cos\theta` and :math:`\sin\theta`
(e.g. terms with even :math:`l`) are allowed.

In fact, though, odd :math:`l` terms are always forbidden because the Stevens factor, :math:`\theta_l`, is zero for odd :math:`l` [#]_.
This implies that, as far as the crystal field is concerned, all magnetic ions lie on centres of inversion.

`Table 2`_ summarises all the allowed crystal field parameters for the crystallographic point groups.
The negative terms in this table relate to the negative :math:`m` tesseral harmonics, defined in equation :eq:`zlm`.
They are thus proportional to :math:`\exp(-i |m| \phi) -(-1)^m \exp(i |m|\phi)`
and thus to :math:`\cos m\phi` or :math:`\sin m\phi` depending on the parity of :math:`m`,
leading to the terms "cosine tesseral harmonics" or "sine tesseral harmonics".
Whether the :math:`m<0` terms are zero or not depends on the secondary symmetry operations.
If the point group only has one symmetry operation (e.g. just :math:`C_n`) or if the only other operation is a horizontal mirror plane,
then the negative :math:`m` term is generally allowed.
This is because the horizontal mirror plane only changes :math:`z` so does not affect the :math:`\phi` coordinate.


Physical properties and INS cross-section
-----------------------------------------

The splitting of the ground state spin-orbit multiplet (in the case of rare earths) also affects the magnetic physical
properties of the material, such as its low temperature heat capacity and magnetisation. In order to account for
magnetic fields, the Zeeman term,

.. math::
      \mathcal{H}_{\mathrm{Z}} &= -\mu_B \mathbf{H} \cdot \left( \mathbf{L} + 2 \mathbf{S} \right) \mathrm{\qquad or\ } \\
                               &= -\mu_B \mathbf{H} \cdot \left( g_J \mathbf{J} \right) ,
  :label: zeeman

where the second equality applies in the case of the rare-earths, where :math:`L+2S=g_J J` and the Landé
:math:`g`-factors for the different trivalent ions are listed in table `Table 1`_. The dot product expresses the sum
where the :math:`x`, :math:`y`, and :math:`z` directions refer to the crystal field coordinates (usually with :math:`z` taken to be
along the axis of highest symmetry), rather than necessarily relating to any crystallographic axes. Thus it may be
necessary to rotate the coordinate systems (or equivalently, the crystal field parameters) for actual calculations for
magnetic fields applied parallel to particular crystallographic directions.
The `FOCUS manual <https://epubs.stfc.ac.uk/manifestation/5723/RAL-TR-95-023.pdf>`_ [#]_
has some details of how these calculations may be accomplished, and further details may be found in reference `[Buckmaster72]`_.

The :math:`\hat{J}_x`, :math:`\hat{J}_y`, :math:`\hat{J}_z` operators may be identified with the :math:`\hat{C}_q^{(k)}` operators for :math:`k=1`
with :math:`x`, :math:`y` and :math:`z` corresponding to :math:`q=1,-1` and 0 respectively [#]_.

The magnetisation may then be calculated from the expectation value of the magnetic moment operator
:math:`\hat{\mathbf{J}} = \hat{\mathbf{L}} + 2\hat{\mathbf{S}} = (\hat{J}_x, \hat{J}_y, \hat{J}_z)`

.. math::
    M(H,T) = \frac{1}{Z} \sum_n \langle \psi_n(\mathbf{H}) | \hat{\mathbf{J}} | \psi_n(\mathbf{H}) \rangle \exp \left(\frac{-E_n(H)}{k_B T}\right),
  :label: magnetisation

where :math:`| \psi_n(\mathbf{H}) \rangle` is the wavefunction or eigenvector of the Hamiltonian containing both crystal and
Zeeman terms at some finite magnetic field :math:`\mathbf{H}`, :math:`E_n` is the corresponding energy or eigenvalue, and the
partition function is

.. math::
   Z = \sum_n \exp(-E_n/k_B T).
  :label: zustand

The heat capacity is the derivative of the internal energy :math:`U` with respect to temperature, where :math:`U` is the expectation
value of the eigenvalues of the Hamiltonian, e.g. :math:`U = \frac{1}{Z}\sum_n E_n \exp(-E_n/k_B T)`.
Thus the heat capacity is

.. math::
    C_v = \frac{1}{k_BT^2} \left\{ -\left(\frac{1}{Z}\sum_n E_n \exp(-\beta E_n)\right)^2 +
    \frac{1}{Z} \sum_n E_n^2 \exp(-\beta E_n) \right\},
    :label: heatcapacity

where :math:`\beta = 1/k_B T`.

Finally, the single-ion neutron scattering transition may also be calculated using the :math:`\hat{J}_{\alpha}` (:math:`\alpha=x,y,z`)
operators as:

.. math::
    I_{n\rightarrow m} = \left(\frac{g_{\mathrm{n}} r_e}{2}\right)^2
            \frac{\exp\left(\frac{-E_n}{k_B T}\right)}{Z} \frac{2}{3}
               \sum_{\alpha} \left| \langle \psi_n |  \hat{J}_{\alpha} | \psi_m \rangle \right|^2,
  :label: neutcf

:math:`I_{n\rightarrow m}` is the intensity in barns per steradian, :math:`g_{\mathrm{n}}` is the neutron's :math:`g`-factor
(:math:`g_n=-3.826`), and :math:`r_e` is the classical electron radius (:math:`r_e=2.82` fm). The product :math:`g_{\mathrm{n}}r_e` represents
the interaction of the neutron's moment with the electrons in the sample. The intensities also obey the sum rule

.. math::
    \sum_{n,m} I_{n\rightarrow m} = \frac{2}{3} \left(\frac{g_{\mathrm{n}} r_e}{2}\right)^2 g_J^2 J(J+1)
  :label: sumrule1

----

.. [#] The notation is confused somewhat in Mantid (and in the literature) by the use of the term "imaginary crystal field parameters" to refer
       to the coefficients of the operators with :math:`m<0` (the operators corresponding to the sine tesseral harmonic functions), because of
       the :math:`i/\sqrt{2}` prefactor of this hermitian combination. The actual parameter value, however, is real.

.. [#]

       The Stevens factor :math:`\theta_l = \langle \nu,L,S,J | |O^{(l)}|| \nu,L,S,J \rangle` is a "reduced matrix element" which can be factorised
       into separate terms involving only each quantum number :math:`J`, :math:`S`, and :math:`L` in turn by using the Wigner-Eckart theorem.
       The final term in this factorisation is the single-electron matrix element of the tensor operator

       .. math::
         \langle l | |T^{(k)}|| l \rangle = (-1)^l l \left( \begin{array}{ccc} l & k & l \\ 0 & 0 & 0 \end{array} \right),
         :label: redmat1e

       where :math:`l` is the orbital quantum number of the single electron (e.g. 3 for :math:`f` electrons).
       The :math:`3j` symbol in this expression is zero unless :math:`k` is even.
       Moreover, the top row of the :math:`3j` symbol must obey the *triangular inequality* which in this case implies that :math:`k<2l`.
       Thus for rare earths, only the terms :math:`k=2,4,6` are needed.
       The :math:`k=0` term is a constant and does not produce any splitting, rather it shifts the energy of all levels by a constant,
       so is usually ignored in neutron spectroscopy (which can only measure the difference between energy levels).
       Note also that the above term is not calculated in Mantid or for the splitting of the ground state multiplet.
       This is because it only contributes to the Stevens factor which, as per equation :eq:`bkq`,
       is usually absorbed into the crystal field parameter :math:`B_q^k`.
       Thus, equation :eq:`redmat1e` implies that, as far as the crystal field is concerned, all magnetic ions lie on centres of inversion.

.. [#] The Mantid crystal field code is a port to C++ of the FOCUS Fortran 77 code by Peter Fabi.

.. [#] Note that equation :eq:`redmat1e` applies only for the orbital part. In this case we apply the rank 1 operator mostly to the spin part where
       the reduced matrix element is non-zero, and this is subsumed into the total angular momentum :math:`J`

Appendix A: Wybourne Normalisation
----------------------------------

It turns out that the spherical harmonic functions :math:`Y_{lm}` are not the most convenient form in which to express the
expansion of the crystal field potential when we want to transform it into a Hamiltonian operator matrix.
Instead, an alternative *normalisation* convention, called the *Wybourne* normalisation after `[Wybourne65]`_,
is used, where the crystal field potential is expressed in terms of the functions

.. math::
    C_{lm} = \sqrt{\frac{4\pi}{2l+1}} Y_{lm}.
  :label: clm

In expressing the crystal field Hamiltonian in terms of the angular momentum :math:`J_z` basis states,
we have to use a set of operators acting on this basis rather than the above functions, which act on atomic positions.
As we mention above, the spherical tensor operators :math:`T_q^(k)` are used
because they transform in the same way under rotations as the :math:`C_{lm}` functions.
What this means is that they obey the same commutation relations with respects to the angular momentum operators:

.. math::
        [J_z,C_{lm}] &= m C_{lm}, \\
        [J_{\pm},C_{lm}] &= \sqrt{(l\mp m)(l\pm m+1)} C_{l,m\pm 1}, \\
        [J_z,T_q^{(k)}] &= q T_q^{(k)}, \\
        [J_{\pm},T_q^{(k)}] &= \sqrt{(k\mp q)(k\pm q+1)} T_{q\pm 1}^{(k)}.

Now, it turns out the matrix elements of the tensor operators can expressed, via the Wigner-Eckart theorem, as the
product of an angular momentum coupling (Clebsch-Gordan) coefficient, and a *reduced matrix element*,

.. math::
    \langle L,L_z | T_q^{(k)}| L,L'_z \rangle = (-1)^{L-L_z}
        \left( \begin{array}{ccc} L & k & L \\ -L_z & q & L'_z \end{array} \right)
    \langle L ||t^{(k)}|| L \rangle,
  :label: tkq

where we have expressed the Clebsch-Gordan coefficient (in the round brackets) as a :math:`3j`-symbol,
and the reduced matrix element :math:`\langle L ||t^{(k)}|| L \rangle`
depends only on the operator rank :math:`k` and the total angular momentum :math:`L`.
Within a single :math:`L`-manifold (that is ignoring other states with different :math:`L`, and just considering the splitting
of the :math:`2L+1` formerly degenerate :math:`L_z` levels), it can be set to `[SmithThornley66]`_

.. math::
    \langle L ||t^{(k)}|| L \rangle = \frac{1}{2^k} \sqrt{\frac{(2L+k+1)!}{(2L-k)!}}.
  :label: redmat

Thus the crystal field Hamiltonian in the Wybourne normalisation is

.. math::
    \mathcal{H}_{\mathrm{CEF}}^{\mathrm{Wybourne}} = \sum_{k=0,2,4,6} \sum_{q=-k}^k D_q^k T_q^{(k)},
  :label: wycf

Note that the :math:`D_q^k` parameters are complex numbers.

We can also construct the Hermitian operators :math:`\hat{C}_q^{(k)}` analogous to the tesseral harmonic functions:

.. math::
    \hat{C}_q^{(k)} = \left\{ \begin{array}{ll}
            \frac{i}{\sqrt{2}} \left[ T_q^{(k)} - (-1)^q T_{-q}^{(k)} \right] & q<0 \\
                                      T_q^{(k)} & q=0 \\
            \frac{1}{\sqrt{2}} \left[ T_{-q}^{(k)} + (-1)^q T_q^{(k)} \right] & q>0 \\
    \end{array} \right..
  :label: ckq

And so construct a "real-valued" Wybourne normalised crystal field Hamiltonian as:

.. math::
    \mathcal{H}_{\mathrm{CEF}}^{\mathrm{RealWybourne}} = \sum_{k=0,2,4,6} \sum_{q=-k}^k L_q^k \hat{C}_q^{(k)},
  :label: realwycf

where the :math:`L_q^k` parameters are real and related to the :math:`D_q^k` parameters by:

.. math::
    D_q^k = \left\{ \begin{array}{ll}
                       (L_{|q|}^k + i L_{-|q|}^k)   &  q < 0  \\
                        L_0^k                       &  q = 0  \\
            (-1)^{|q|} (L_{|q|}^k - i L_{-|q|}^k)   &  q > 0
    \end{array} \right.,
  :label: real2imagwy

Note that the operators :math:`\hat{C}_q^{(k)}` are *not* the Stevens operators :math:`O_q^{(k)}`.
This is because although Stevens constructed his operators from the tesseral harmonics functions,
he omits the prefactors of those equations using only the parts containing the :math:`x`, :math:`y`, :math:`z` coordinates.
The :math:`\hat{C}_q^{(k)}` *does* contain the prefactors, so are related to the Stevens operators by:

.. math::
    O_q^{(k)} = \lambda_{k,|q|} \hat{C}_q^{(k)}
  :label: wy2stev

where the ratios :math:`\lambda_{k,|q|}` are summarised in `Table 3`_.

The crystal field in Stevens normalisation used in Mantid is then defined by:

.. math::
    \mathcal{H}_{\mathrm{CEF}}^{\mathrm{StevensNeutron}} = \sum_{k=0,2,4,6} \sum_{q=-k}^k B_q^k O_q^{(k)},
  :label: stevneutcf

so the Stevens :math:`A_q^k` and :math:`B_q^k` parameters are related to the real-valued Wybourne parameters by:

.. math::
    A_q^k &= \lambda_{k,|q|} L_q^k / \langle r^k \rangle \\
    B_q^k &= \lambda_{k,|q|} \theta_k L_q^k

where :math:`\theta_k` are the Stevens operator equivalent factors tabulated in `Table 1`_.

----

Appendix B: Tables
------------------

.. |alpha| replace:: :math:`(\alpha=\theta_2)\times10^2`
.. |beta| replace:: :math:`(\beta=\theta_4)\times10^4`
.. |gamma| replace:: :math:`(\gamma=\theta_6)\times10^6`
.. |half| replace:: :math:`\frac{1}{2}`
.. |3half| replace:: :math:`\frac{3}{2}`
.. |5half| replace:: :math:`\frac{5}{2}`
.. |7half| replace:: :math:`\frac{7}{2}`
.. |9half| replace:: :math:`\frac{9}{2}`
.. |15half| replace:: :math:`\frac{15}{2}`

.. _Table 1:

+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
|           Ion            | L |   S     |    J     | :math:`g_J`          | |alpha| | |beta| | |gamma| |
+==========================+===+=========+==========+======================+=========+========+=========+
| :math:`\mathrm{Ce}^{3+}` | 3 | |half|  | |5half|  | :math:`\frac{6}{7}`  | -5.714  | 63.49  |  0      |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Pr}^{3+}` | 5 | 1       | 4        | :math:`\frac{4}{5}`  | -2.101  | -7.346 |  60.99  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Nd}^{3+}` | 6 | |3half| | |9half|  | :math:`\frac{8}{11}` | -0.6428 | -2.911 | -37.99  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Pm}^{3+}` | 6 | 2       | 4        | :math:`\frac{3}{5}`  |  0.7714 |  4.076 |  60.89  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Sm}^{3+}` | 5 | |5half| | |5half|  | :math:`\frac{2}{7}`  |  4.127  | 25.01  |  0      |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Eu}^{3+}` | 3 | 3       | 0        |                      |         |        |         |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Gd}^{3+}` | 0 | |7half| | |7half|  | 2                    |         |        |         |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Tb}^{3+}` | 3 | 3       | 6        | :math:`\frac{3}{2}`  | -1.0101 |  1.224 | -1.121  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Dy}^{3+}` | 5 | |5half| | |15half| | :math:`\frac{4}{3}`  | -0.6349 | -0.592 |  1.035  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Ho}^{3+}` | 6 | 2       | 8        | :math:`\frac{5}{4}`  | -0.2222 | -0.333 | -1.294  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Er}^{3+}` | 6 | |3half| | |15half| | :math:`\frac{6}{5}`  |  0.2540 |  0.444 |  2.070  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Tm}^{3+}` | 5 | 1       | 6        | :math:`\frac{7}{6}`  |  1.0101 |  1.632 | -5.606  |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+
| :math:`\mathrm{Yb}^{3+}` | 3 | |half|  | |7half|  | :math:`\frac{8}{7}`  |  3.175  | -17.32 | 148.0   |
+--------------------------+---+---------+----------+----------------------+---------+--------+---------+

Table 1: *Total angular momentum quantum numbers* :math:`L`, :math:`S` and :math:`J`, *Landé* :math:`g_J` *factors,
and Stevens factors* :math:`\theta_k` *for the ground states of the trivalent rare-earth ions*.
After `[JensenMackintosh91]`_.
The ground state of :math:`\mathrm{Gd}^{3+}` is a pure spin state, on which the crystal field does not operate.
Eu compound often do not adopt the trivalent state, and Pm is radioactive so not much studied.

.. |m21| replace:: :math:`\sqrt{6}`
.. |m22| replace:: :math:`\frac{1}{2}\sqrt{6}`
.. |m41| replace:: :math:`\frac{1}{2}\sqrt{5}`
.. |m42| replace:: :math:`\frac{1}{4}\sqrt{10}`
.. |m43| replace:: :math:`\frac{1}{2}\sqrt{35}`
.. |m44| replace:: :math:`\frac{1}{8}\sqrt{70}`
.. |m61| replace:: :math:`\frac{1}{8}\sqrt{42}`
.. |m62| replace:: :math:`\frac{1}{16}\sqrt{105}`
.. |m63| replace:: :math:`\frac{1}{8}\sqrt{105}`
.. |m64| replace:: :math:`\frac{3}{16}\sqrt{14}`
.. |m65| replace:: :math:`\frac{3}{8}\sqrt{77}`
.. |m66| replace:: :math:`\frac{1}{16}\sqrt{231}`

.. _Table 2:

.. |pm| replace:: :math:`\pm`
.. |p| replace:: :math:`+`
.. |B20| replace:: :math:`\mathrm{B}_2^0`
.. |B21| replace:: :math:`\mathrm{B}_2^{\pm 1}`
.. |B22| replace:: :math:`\mathrm{B}_2^{\pm 2}`
.. |B40| replace:: :math:`\mathrm{B}_4^0`
.. |B41| replace:: :math:`\mathrm{B}_4^{\pm 1}`
.. |B42| replace:: :math:`\mathrm{B}_4^{\pm 2}`
.. |B43| replace:: :math:`\mathrm{B}_4^{\pm 3}`
.. |B44| replace:: :math:`\mathrm{B}_4^{\pm 4}`
.. |B60| replace:: :math:`\mathrm{B}_6^0`
.. |B61| replace:: :math:`\mathrm{B}_6^{\pm 1}`
.. |B62| replace:: :math:`\mathrm{B}_6^{\pm 2}`
.. |B63| replace:: :math:`\mathrm{B}_6^{\pm 3}`
.. |B64| replace:: :math:`\mathrm{B}_6^{\pm 4}`
.. |B65| replace:: :math:`\mathrm{B}_6^{\pm 5}`
.. |B66| replace:: :math:`\mathrm{B}_6^{\pm 6}`
.. |CiC1| replace:: :math:`\mathrm{C}_i,\  \mathrm{C}_1`
.. |C2CsC2h| replace:: :math:`\mathrm{C}_2,\  \mathrm{C}_s,\  \mathrm{C}_{2h}`
.. |C2vD2D2h| replace:: :math:`\mathrm{C}_{2v},\  \mathrm{D}_2,\  \mathrm{D}_{2h}`
.. |C4S4C4h| replace:: :math:`\mathrm{C}_4,\  \mathrm{S}_4,\  \mathrm{C}_{4h}`
.. |D4C4vD4h| replace:: :math:`\mathrm{D}_4,\  \mathrm{C}_{4v},\  \mathrm{D}_{2d},\  \mathrm{D}_{4h}`
.. |C3S6| replace:: :math:`\mathrm{C}_3,\  \mathrm{S}_6`
.. |D3C3vD3d| replace:: :math:`\mathrm{D}_3,\  \mathrm{C}_{3v},\  \mathrm{D}_{3d}`
.. |C6C3hC6h| replace:: :math:`\mathrm{C}_6,\  \mathrm{C}_{3h},\  \mathrm{C}_{6h}`
.. |D6C6vD6h| replace:: :math:`\mathrm{D}_6,\  \mathrm{C}_{6v},\  \mathrm{D}_{3h},\  \mathrm{D}_{6h}`
.. |TTh| replace:: :math:`\mathrm{T},\  \mathrm{T}_{h}`
.. |TdOOh| replace:: :math:`\mathrm{T}_d,\ \mathrm{O},\  \mathrm{O}_{h}`

+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| symmetry   | point group | |B20| | |B21| | |B22| | |B40| | |B41| | |B42| | |B43| | |B44| | |B60| | |B61| | |B62| | |B63| | |B64| | |B65| | |B66| |
+============+=============+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+
| triclinic  | |CiC1|      | |p|   | |pm|  | |pm|  | |p|   | |pm|  | |pm|  | |pm|  | |pm|  | |p|   | |pm|  | |pm|  | |pm|  | |pm|  | |pm|  | |pm|  |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| monoclinic | |C2CsC2h|   | |p|   |       | |pm|  | |p|   |       | |pm|  |       | |pm|  | |p|   |       | |pm|  |       | |pm|  |       | |pm|  |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| rhombic    | |C2vD2D2h|  | |p|   |       | |p|   | |p|   |       | |p|   |       | |p|   | |p|   |       | |p|   |       | |p|   |       | |p|   |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| tetragonal | |C4S4C4h|   | |p|   |       |       | |p|   |       |       |       | |pm|  | |p|   |       |       |       | |pm|  |       |       |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| tetragonal | |D4C4vD4h|  | |p|   |       |       | |p|   |       |       |       | |p|   | |p|   |       |       |       | |p|   |       |       |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| trigonal   | |C3S6|      | |p|   |       |       | |p|   |       |       | |pm|  |       | |p|   |       |       | |pm|  |       |       | |pm|  |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| trigonal   | |D3C3vD3d|  | |p|   |       |       | |p|   |       |       | |p|   |       | |p|   |       |       | |p|   |       |       | |p|   |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| hexagonal  | |C6C3hC6h|  | |p|   |       |       | |p|   |       |       |       |       | |p|   |       |       |       |       |       | |pm|  |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| hexagonal  | |D6C6vD6h|  | |p|   |       |       | |p|   |       |       |       |       | |p|   |       |       |       |       |       | |p|   |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| cubic      | |TTh|       |       |       |       | |p|   |       |       |       | |p|   | |p|   |       | |p|   |       | |p|   |       | |p|   |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| cubic      | |TdOOh|     |       |       |       | |p|   |       |       |       | |p|   | |p|   |       |       |       | |p|   |       |       |
+------------+-------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+

Table 2: *Possible local symmetries and corresponding nonzero CEF parameters.*
':math:`+`' indicates only :math:`|m|` terms are nonzero.
':math:`\pm`' indicates that :math:`-|m|` terms are also non-zero.
In the case when :math:`m>0` and both parameters :math:`B_l^m` and :math:`B_l^{-m}` are nonzero,
one of these :math:`B_l^m` with :math:`m>0` can by made zero by a rotation of the coordinate system.
However, the appropriate orientation of the coordinate system in these cases is not known *a priori*.
It requires the knowledge of the CEF parameters.
Note, that for cubic symmetry additionally :math:`\mathrm{B}_4^4=5 \mathrm{B}_4^0,` and :math:`\mathrm{B}_6^4 = -21 \mathrm{B}_6^0`.

.. _Table 3:

+-----+----------------------+-------+-------+-------+-------+-------+-------+
| *l* | :math:`|m|=0`        |   1   |   2   |   3   |   4   |   5   |   6   |
+=====+======================+=======+=======+=======+=======+=======+=======+
|  2  | :math:`\frac{1}{2}`  | |m21| | |m22| |       |       |       |       |
+-----+----------------------+-------+-------+-------+-------+-------+-------+
|  4  | :math:`\frac{1}{8}`  | |m41| | |m42| | |m43| | |m44| |       |       |
+-----+----------------------+-------+-------+-------+-------+-------+-------+
|  6  | :math:`\frac{1}{16}` | |m61| | |m62| | |m63| | |m64| | |m65| | |m66| |
+-----+----------------------+-------+-------+-------+-------+-------+-------+

Table 3: *Ratios* :math:`\lambda_{lm}` *of the Stevens to the real valued Wybourne normalised parameters.*
After `[NewmanNg00]`_.


References
----------

.. _[Buckmaster72]:

[Buckmaster72] `H. A. Buckmaster, R. Chatterjee, and Y. H. Shing, phys. stat. sol.  (a) 13, 9 (1972). <https://doi.org/10.1002/pssa.2210130102>`_

.. _[Hutchings64]:

[Hutchings64] `M. T. Hutchings, in Solid State Physics, edited by F. Seitz and D. Turnbull (Academic Press, New York, 1964), vol. 16, pp.  227–273. <https://doi.org/10.1016/S0081-1947(08)60517-2>`_

.. _[JensenMackintosh91]:

[JensenMackintosh91] `J. Jensen and A. R. Mackintosh, Rare Earth Magnetism (Clarendon Press, 1991). <https://www.fys.ku.dk/~jjensen/REM.htm>`_

.. _[Judd63]:

[Judd63] B. R. Judd, Operator Techniques in Atomic Spectroscopy (McGraw-Hill, 1963), reprinted (1998) by Princeton University Press.

.. _[NewmanNg00]:

[NewmanNg00] D. J. Newman and B. K. C. Ng, Crystal Field Handbook (Cambridge University Press, 2000).

.. _[SmithThornley66]:

[SmithThornley66] `D. Smith and J. H. M. Thornley, Proc. Phys. Soc. 89, 779 (1966) <https://doi.org/10.1088/0370-1328/89/4/301>`_

.. _[Stevens52]:

[Stevens52] `K. W. H. Stevens, Proc. Phys. Soc. A 65, 209 (1952). <https://doi.org/10.1088/0370-1298/65/3/308>`_

.. _[Wybourne65]:

[Wybourne65] B. G. Wybourne, Spectroscopic Properties of Rare Earths (Interscience, New York, 1965).


.. categories:: Concepts
