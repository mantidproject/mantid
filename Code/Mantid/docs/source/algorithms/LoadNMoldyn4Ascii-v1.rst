.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads data from version 4 of nMOLDYN saved in the ASCII format after being
extracted from the ``.tar`` archive.

Usage
-----

**Example - Loading a simulation from a nMOLDYN 4 data file.**

.. code-block:: python

    data = LoadNMoldyn4Ascii(Directory='~/nmoldyn4_data',
                             Functions=['sqf_total', 'iqt_total'])

    for ws in data:
        print ws.name()

Output:

.. code-block:: python

    sqf_total
    iqt_total

.. categories::

.. sourcelink::
