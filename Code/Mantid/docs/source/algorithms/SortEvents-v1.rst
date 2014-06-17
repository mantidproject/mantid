.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

In an `EventWorkspace <EventWorkspace>`__, event binning is performed on
the fly. The algorithm for binning requires a list of events sorted by
time of flight, so it will perform a sort (once) on each pixel -
however, this is done on request and without using multiple CPUs). To
speed up the calculation, the Sort algorithm pre-sorts by Time of
Flight, using multiple CPUs. Using this algorithm is completely
optional.

.. categories::
