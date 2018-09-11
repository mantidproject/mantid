.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes an input workspace (the LHSWorkspace) and removes
from it's list of peaks any that are also found in a second workspace
(the RHSWorkspace) in the same position in Q space within the specified
tolerance. Each peak in the RHSWorkspace is taken in turn and compared
to each peak in the LHSWorkspace in turn. The first match encountered is
used, and the matching peak removed from the output before moving onto
the next RHSWorkspace peak.

.. categories::

.. sourcelink::
