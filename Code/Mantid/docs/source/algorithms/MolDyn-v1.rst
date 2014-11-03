.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is a loader from simulation data from the nMOLDYN software,
simulations can be loaded form either a plain ASCII file or CDL file.

When loading from a CDL file one or multiple functions can be loaded, when
loading a single function an instrument resolution workspace can be provided
which the loaded function is convoluted with to allow comparison with actual
instrument data.

When loading from ASCII function selection and convolution with an instrument
resolution are unavailable.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading a simulation from a CDL file..**

.. testcode:: ExLoadCDLFile

    out_ws_group = MolDyn(Filename='NaF_DISF.cdl', Functions=['Fqt-total', 'Sqw-total'])

    for ws_name in out_ws_group.getNames():
      print ws_name

Output:

.. testoutput:: ExLoadCDLFile

    NaF_DISF_Fqt-total
    NaF_DISF_Sqw-total

.. categories::
