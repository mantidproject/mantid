.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Simulate a USANS workspace. This algorithm is used to test the USANS
reduction during development while no real data is available.

A matrix workspace is created for a given analyzer angle. A list of
wavelength peaks coming out of the monochromator can be specified. The
width of those peaks can also be specified.

Both the main detector and the transmission detector are filled with
compatible signals according to a dummy transmission curve.

The amplitude of the signal in the main detector is given by a sphere
model.

A monitor workspace is created with a fake beam profile.

**Note**: This algorithm can take a very long time to execute and is
of limited use for non-developers.

.. categories::

.. sourcelink::
