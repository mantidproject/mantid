.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This SANS workflow algorithm will compute the sensitivity correction
from a given flood field data set. It will apply the proper corrections
to the data according the the input property manager object. Those
corrections may include dark current subtraction, moving the beam
center, the solid angle correction, and applying a patch.

If an input workspace is given, the computed correction will be applied
to that workspace.

A Nexus file containing a pre-calculated sensitivity correction can also
be supplied for the case where we simply want to apply the correction to
an input workspace.

.. categories::
