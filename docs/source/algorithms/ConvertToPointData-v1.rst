.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The ``InputWorkspace`` must contain histogram data. Once executed the
``OutputWorkspace`` will contain point data, where the X values are simply
the centre points of the input bins.

Note that when this is applied to a ``RebinnedOutput`` workspace created
by :ref:`algm-SofQWNormalisedPolygon` the bin fractions information is
discarded (set to ``1``) even if the workspace is subsequently converted
back to a histogram. This means that error propagation after conversion
may not be correct. More information on ``RebinnedOutput`` workspaces can be
found on :ref:`FractionalRebinning <FractionalRebinning>`.

.. categories::

.. sourcelink::
