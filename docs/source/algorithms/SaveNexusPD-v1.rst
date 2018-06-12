
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The file created by this algorithm saves a single workspace in a
single ``NXentry`` of a `NeXus <http://www.nexusformat.org/>`_
file. The x-axes are saved in time-of-flight, d-spacing, and
(optionally) momentum transfer. The file is formated to support the
rules for `NXdata
<http://download.nexusformat.org/doc/html/classes/base_classes/NXdata.html>`_,
`NXdetector
<http://download.nexusformat.org/doc/html/classes/base_classes/NXdetector.html>`_,
and `simple plotting
<http://download.nexusformat.org/doc/html/examples/h5py/index.html#plotting-the-hdf5-file>`_. Fields
that appear to be in the file twice are actually hard links to the
same object. The format of the file (for a workspace with a single spectrum is described with the tree (starting inside the ``NXentry``)

* ``NXinstrument``

  * ``NXmoderator``

    * ``distance`` - commonly refered to as ``L1`` in diffraction

  * ``NXdetector``

    * ``data``

    * ``errors``

    * ``tof``

    * ``dspacing``

    * ``Q`` - values are stored in reverse so they are parallel to the
      ``data`` array

    * ``distance`` - commonly refered to as ``L2`` in diffraction

    * ``polar_angle`` - commonly refered to as two-theta in diffraction

    * ``azimuthal_angle`` - out of plane angle

* ``NXdata``

  * ``data``

  * ``errors``

  * ``tof``

  * ``dspacing``

  * ``Q``

* ``proton_charge`` the accumulated charge for the data

A description of the `NeXus coordinate system
<http://download.nexusformat.org/doc/html/design.html#nexus-coordinate-systems>`_
contains images describing the coordinates.

.. categories::

.. sourcelink::
