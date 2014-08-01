.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL TOF NeXus file into a :ref:`Workspace2D <Workspace2D>` with
the given name.

This loader calculates the elastic peak position (EPP) on the fly. In
cases where the dispersion peak might be higher than the EPP, it is good
practice to load a Vanadium file.

The property FilenameVanadium is optional. If it is present the EPP will
be loaded from the Vanadium data.

To date this algorithm only supports: IN4, IN5 and IN6

.. categories::
