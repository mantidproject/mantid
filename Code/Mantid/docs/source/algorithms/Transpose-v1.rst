.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm transposes a workspace, so that an N1 x N2 workspace
becomes N2 x N1.

The X-vector values for the new workspace are taken from the axis values
of the old workspace, which is generaly the spectra number but can be
other values, if say the workspace has gone through
:ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`.

.. warning::

    The new axis values are taken from the previous X-vector values for the
    first specrum in the workspace. For this reason, use with ragged
    workspaces is undefined.

.. categories::
