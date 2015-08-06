.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads data from version 4 of nMOLDYN saved in the ASCII format after being
extracted from the ``.tar`` archive.

Functions can be provided with ot without the comma between multiple dependant
variables, for example a function names ``f(q,t_H)`` in nMOLDYN can be loaded
using either ``f(q,t_H)`` or ``f(qt)_H`` as a function name in this algorithm.

Assumptions on data format
--------------------------

The ``Directory`` property must be given the directory that is produced when you
extract the ``.tar`` archive from nMMOLDYN without modifications which must only
contain the data files produces from a single export operation from nMOLDYN.

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
