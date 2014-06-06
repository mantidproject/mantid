.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Masks bins in a workspace. The user specified masking parameters,
including spectra, xmin and xmax are given via a TableWorkspace.

It calls algorithm MaskBins underneath.

Related Algorithms
------------------

MaskBins
########

:ref:`algm-MaskBins` masks bins in a workspace. Masked bins should
properly be regarded as having been completely removed from the
workspace. Bins falling within the range given (even partially) are
masked, i.e. their data and error values are set to zero and the bin is
added to the list of masked bins. This range is masked for all spectra
in the workspace (though the workspace does not have to have common X
values in all spectra).

.. categories::
