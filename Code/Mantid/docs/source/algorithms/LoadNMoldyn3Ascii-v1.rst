.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is a loader from simulation data saved by version 3 of the
nMOLDYN package, simulations can be loaded form either a plain ASCII (``.dat``)
file or ``.cdl`` file.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading a simulation from a CDL file.**

.. testcode:: ExLoadCDLFile

    out_ws_group = LoadNMoldyn3Ascii(Filename='NaF_DISF.cdl',
                                     Functions=['Fqt-total', 'Sqw-total'])

    for ws_name in out_ws_group.getNames():
      print ws_name

Output:

.. testoutput:: ExLoadCDLFile

    NaF_DISF_Fqt-total
    NaF_DISF_Sqw-total

.. categories::

.. sourcelink::
