.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm uses a numerical integration method to calculate
attenuation factors resulting from absorption and single scattering in a
cylindrical sample with the dimensions and material properties given.
Factors are calculated for each spectrum (i.e. detector position) and
wavelength point, as defined by the input workspace. The sample is
divided up into a stack of slices, which are then divided into annuli
(rings). These annuli are further divided (see Ref. [2], Appendix A) to
give the full set of elements for which a calculation will be carried
out. Thus the calculation speed depends linearly on the total number of
bins in the workspace and on the number of slices. The dependence on the
number of annuli is stronger, going as :math:`3n ( n+1 )`.

Path lengths through the sample are then calculated for the centre-point
of each element and a numerical integration is carried out using these
path lengths over the volume elements.

Assumptions
###########

Although no assumptions are made about the beam direction or the sample
position, the cylinder will be constructed with its centre at the sample
position and it's axis along the y axis (which in the case of most
instruments is the vertical).

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
`instrument <instrument>`__ associated with the workspace must be fully
defined because detector, source & sample position are needed.

References
##########

The method used here is based upon work presented in the following two
papers, although it does not yet fully implement all aspects discussed
there (e.g. there's no multiple scattering and no concentric cylinders).

#. I.A. Blech & B.L. Averbach, *Multiple Scattering of Neutrons in
   Vanadium and Copper*, Phys. Rev. **137 4A** (1965) A1113.
#. A.K. Soper & P.A. Egelstaff, *Multiple Scattering and Attenuation of
   Neutrons in Concentric Cylinders*, NIM **178** (1980) 415.

.. categories::
