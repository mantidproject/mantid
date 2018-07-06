.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads data from version 4 of nMOLDYN saved in the ASCII format after being
extracted from the ``.tar`` archive.

Functions can be provided with ot without the comma between multiple dependant
variables, for example a function names ``f(q,t_H)`` in nMOLDYN can be loaded
using either ``f(q,t)_H`` or ``f(qt)_H`` as a function name in this algorithm.

Assumptions on data format
--------------------------

The ``Directory`` property must be given the directory that is produced when you
extract the ``.tar`` archive from nMOLDYN without modifications which must only
contain the data files produces from a single export operation from nMOLDYN.

Axis Unit Conversions
---------------------

When loading certain axis from nMOLDYN 4 the units may be converted to an
equivalent unit in Mantid. The possible conversions are shown in the table
below:

+-----------+---------+------------------+--------------+
| nMOLDYN             | Mantid                          |
+-----------+---------+------------------+--------------+
| name      | unit    | name             | unit         |
+===========+=========+==================+==============+
| frequency | THz     | Energy           | meV          |
+-----------+---------+------------------+--------------+
| q         | nm**-1  | MomentumTransfer | Angstrom**-1 |
+-----------+---------+------------------+--------------+
| Time      | pSecond | TOF              | uSecond      |
+-----------+---------+------------------+--------------+

Usage
-----

**Example - Loading a simulation from a nMOLDYN 4 data file.**

.. code-block:: python

    data = LoadNMoldyn4Ascii(Directory='~/nmoldyn4_data',
                             Functions=['sqf_total', 'iqt_total'])

    for ws in data:
        print(ws.name())

Output:

.. code-block:: python

    sqf_total
    iqt_total

.. categories::

.. sourcelink::
