.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves the data in a workspace into a file in the ASCII 'SPE' format (as
described `here <Media:Spe_file_format.pdf>`__).

The units used for saving will match those of the input workspace, such
that if you have the units Momentum Transfer ('DeltaE') then you will
get a traditional SPE file, you could choose to have the units in mod Q
and then it will save to an SPQ file variant.

Restrictions on the input workspace
###################################

The input workspace must contain histogram data with common binning on
all spectra.

.. categories::
