.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Attempts to load information about the instrument from a ISIS NeXus
file. In particular attempt to read L2 and 2-theta detector position
values and add detectors which are positioned relative to the sample in
spherical coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also adds dummy
source and samplepos components to instrument.

LoadInstrumentFromNexus is intended to be used as a child algorithm of
other Loadxxx algorithms, rather than being used directly. It is used by
LoadMuonNexus version 1.

.. categories::
