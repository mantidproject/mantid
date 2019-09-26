.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a deep copy of all of the information in the
workspace. It maintains events if the input is an
:ref:`EventWorkspace <EventWorkspace>`. It will call CloneMDWorkspace for a
:ref:`MDEventWorkspace <MDWorkspace>` or a
:ref:`MDHistoWorkspace <MDHistoWorkspace>`. It can also clone a
:ref:`PeaksWorkspace <PeaksWorkspace>`.

If in-place operation is requested (e.g. ``InputWorkspace==OutputWorkspace``) this algorithm does nothing.

.. categories::

.. sourcelink::
