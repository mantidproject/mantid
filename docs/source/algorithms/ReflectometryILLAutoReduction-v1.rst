.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the reduction work-flow algorithm for the ILL reflectometers D17 and FIGARO.
A various number of corresponding direct and reflected angles can be processed.
Some input properties like the background indices except either a single entry for all angles, i.e. number of reflected or direct beams, or an entry for each corresponding angle.
Please note, however, to not delete direct beam work-spaces for efficient re-use for further reductions.
The input runs must be NeXus files (file ending .nxs) and different angles will be comma separated while merging angles will be performed when giving a *+*.

Work-flow
---------

The processing of the direct and reflected beams are shown in the following diagrams.

.. diagram:: ReflectometryILLAutoReduction1-v1_wkflw.dot

.. diagram:: ReflectometryILLAutoReduction2-v1_wkflw.dot

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReflectometryILLAutoReduction1**

.. testcode:: ReflectometryILLAutoReduction1

.. testoutput:: ReflectometryILLAutoReduction1

.. categories::

.. sourcelink::
