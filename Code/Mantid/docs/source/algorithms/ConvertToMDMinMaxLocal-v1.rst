.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculate min-max input values for selected workspace and MD transformation, 
choosen from `MD Transformation factory <MD_Transformation_factory>`_.

Used as helper algorithm for :ref:`algm-ConvertToMD` but can aslo be deployed separately 
to evaluate the MD transformation limits for the current workspace.

Initiates the same as :ref:`algm-ConvertToMD` algorithm transformation from the
`MD Transformation factory <MD_Transformation_factory>`_ and uses this 
transformation to evaluate all points where the transformation can achieve extrema 
for each workspace spectra. Then goes through all extrema points, calculates min/max 
values for each spectra and select global min-max transformation values for 
this workspace.

For example, given input workspace in the units of energy transfer and
requesting :math:`|Q|` inelastic transformation, the algorithm looks through
all spectra of the input workspace and identifies minimal, maximal and
an extremal [#f1]_ energy transfer for the input spectra. Then it runs 
:math:`|Q|,dE` conversion for these energy transfer points and loops through all
spectra of the workspace to identify :math:`|Q|_{min}, |Q|_{max}` and 
:math:`dE_{min},dE_{max}` values.

.. rubric:: Footnotes

.. [#f1] extremal energy transfer for **|Q|** transformation occurs at some
   energy transfer where momentum transfer is maximal. It depends on
   polar angle of the detector.

   
.. categories::
