.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Helper algorithm to calculate min-max input values for ConvertToMD
algorithm, using ConvertToMD algorithm factory.

Initiates the same as ConvertToMD algorithm transformation from the
ConvertToMD factory and uses this transformation to evaluate all points
where the transformation can achieve extrema for each workspace spectra.
Then goes through all extrema points, calculates min/max values for each
spectra and select global min-max values for the whole workspace.

For example, given input workspace in the units of energy transfer and
requesting \|Q\| inelastic transformation, the algorithm looks through
all spectra of the input workspace and identifies minimal, maximal and
an extremal\* energy transfer for the input spectra. Then it runs \|Q\|
dE conversion for these energy transfer points and loops through all
spectra of the workspace to identify \|Q\|\_min, \|Q\|\_max and dE\_min
and dE\_max values.

-  extremal energy transfer for \|Q\| transformation occurs at some
   energy transfer where momentum transfer is maximal. It depends on
   polar

.. categories::
