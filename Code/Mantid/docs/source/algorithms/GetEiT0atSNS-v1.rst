.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Get Ei and T0 on ARCS and SEQUOIA instruments. It accounts for the
following:

-  in the ADARA framework, the monitors are in the first frame.
-  SEQUOIA has event based monitors.
-  some data aquisition errors will create unphysical monitor IDs. This
   will be ignored
-  when vChTrans is 2, on ARCS and SEQUOIA there is no chopper in the
   beam (white beam). Will return not a number for both Ei and T0

.. categories::
