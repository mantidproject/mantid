.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is a loader from simulation data saved by version 3 of the
nMOLDYN package, simulations can be loaded form either a plain ASCII (``.dat``)
file or ``.cdl`` file.

Currently this supports loading :math:`S(Q, \omega)` and :math:`F(Q, t)`
functions from ``.cdl`` files, :math:`F(Q, t)` from ``.dat`` files and any 2D
function (such as density of states) from ``.dat`` files.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading a simulation from a CDL file.**

.. testcode:: ExLoadCDLFile

    out_ws_group = LoadNMoldyn3Ascii(Filename='NaF_DISF.cdl',
                                     Functions=['Fqt-total', 'Sqw-total'])

    for ws_name in out_ws_group.getNames():
      print(ws_name)

Output:

.. testoutput:: ExLoadCDLFile

    NaF_DISF_Fqt-total
    NaF_DISF_Sqw-total

.. categories::

.. sourcelink::
