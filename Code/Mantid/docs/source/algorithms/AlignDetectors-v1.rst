.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The offsets are a correction to the dSpacing values and are applied
during the conversion from time-of-flight to dSpacing as follows:

.. math:: d = \frac{h}{2m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})

The detector offsets can be obtained from either: an
`OffsetsWorkspace <OffsetsWorkspace>`__ where each pixel has one value,
the offset; or a .cal file (in the form created by the ARIEL software).

**Note:** the workspace that this algorithms outputs is a `Ragged
Workspace <Ragged Workspace>`__.

Restrictions on the input workspace
###################################

The input workspace must contain histogram or event data where the X
unit is time-of-flight and the Y data is raw counts. The
`instrument <instrument>`__ associated with the workspace must be fully
defined because detector, source & sample position are needed.

.. categories::
