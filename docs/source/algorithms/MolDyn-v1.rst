.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to load and process simualtion data from the nMOLDYN
package.

Currently this supports loading the ``.cdl`` and ``.dat`` files created by
version 3 of nMOLDYN (using the :ref:`LoadNMoldyn3Ascii
<algm-LoadNMoldyn3Ascii>` algorithm).

When loading from a ``.cdl`` file from nMOLDYN 3 one or multiple functions can
be loaded, when loading a single function an instrument resolution workspace can
be provided which the loaded function is convoluted with to allow comparison
with actual instrument data.

When loading from a ``.dat`` file from nMOLDYN 3 function selection and
convolution with an instrument resolution are unavailable.

Workflow
========

.. diagram:: MolDyn-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading a simulation from a CDL file.**

.. testcode:: ExLoadCDLFile

    out_ws_group = MolDyn(Data='NaF_DISF.cdl',
                          Functions=['Fqt-total', 'Sqw-total'])

    for ws_name in out_ws_group.getNames():
      print(ws_name)

Output:

.. testoutput:: ExLoadCDLFile

    NaF_DISF_Fqt-total
    NaF_DISF_Sqw-total

.. categories::

.. sourcelink::
