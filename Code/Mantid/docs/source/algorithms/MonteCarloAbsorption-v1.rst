.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a Monte Carlo simulation to calculate the
attenuation factor of a given sample for an arbitrary arrangement of
sample + container shapes. The algorithm proceeds as follows for each
spectra in turn:

-  A random point within the sample+container set up is selected and
   chosen as a scattering point;
-  This point then defines an incoming vector from the source position
   and an outgoing vector to the final detector;
-  The total attenuation factor for this path is then calculated as the
   product of the factor for each defined material of the
   sample/container that the track passes through.

Known limitations
-----------------

-  Only elastic scattering is implemented at the moment.

-  The object's bounding box is used as a starting point for selecting a
   random point. If the shape of the object is peculiar enough not to
   occupy much of the bounding box then the generation of the initial
   scatter point will fail.

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
`instrument <instrument>`__ associated with the workspace must be fully
defined because detector, source & sample position are needed.

At a minimum, the input workspace must have a sample shape defined.

.. categories::
