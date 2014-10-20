.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

TODO

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading a simulation from a CDL file..**

.. testcode:: ExLoadCDLFile

    out_ws_group = MolDyn(SampleFile='DISF_NaF.cdl', Functions=['Fqt-total', 'Fqt-Na'])

    for ws_name in out_ws_group.getNames():
      print ws_name

Output:

.. testoutput:: ExLoadCDLFile

    DISF_NaF_Fqt-total
    DISF_NaF_Fqt-Na

.. categories::
