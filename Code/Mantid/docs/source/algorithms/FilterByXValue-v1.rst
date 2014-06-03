.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm filters events outside of the given values (in whatever
units the workspace possesses). This can be a one or two-sided filter
depending on which of xmin & xmax are given. This algorithm pays no
attention whatsoever to any binning that has been set on the input
workspace (though it will be carried over to the output). If you need to
affect the bin boundaries as well, or want to remove some
spectra/pixels, consider using :ref:`algm-CropWorkspace`
instead.

.. categories::
