.. _Instrument:

Instrument
==========

What are they?
--------------

The Instrument is a geometrical description of the components that make
up the beam line. The components described will generally include:

-  The source
-  The sample position
-  Each detector 'pixel'
-  Each monitor

Other components may also be included such as

-  Slits
-  Mirrows
-  Guides
-  Choppers
-  Engineering obstacles in the beam path
-  Link between log-files and variable parameters of the instrument
   (such as the height of a detector table)

An instrument is described using an :ref:`instrument definition
file <InstrumentDefinitionFile>`.

The Mantid geometry is further explained :ref:`here <Geometry>`.

Why do we have a full instrument description, and not just a list of L2 and 2Theta values?
------------------------------------------------------------------------------------------

A list of L2 and 2Theta values will provide information to perform unit
conversions and several other algorithms, however a full geometric
instrument description allows much more.

-  Visualization of the instrument internals with data overlays
-  Complex absorption corrections
-  Montecarlo simulations of experiments
-  Updating the instrument geometry according to values stored in
   log-files



.. categories:: Concepts