.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a deep copy of all of the information in the
workspace. It maintains events if the input is an
:py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>`. It will call CloneMDWorkspace for a
:ref:`MDEventWorkspace <MDWorkspace>` or a
:py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>`. It can also clone a
:py:obj:`PeaksWorkspace <mantid.dataobjectsPeaksWorkspace>`.

If in-place operation is requested (e.g. ``InputWorkspace==OutputWorkspace``) this algorithm does nothing.

.. categories::

.. sourcelink::
