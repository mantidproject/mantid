.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the reduction work-flow algorithm for the ILL reflectometers D17 and FIGARO.
A various number of corresponding direct and reflected angles can be processed.
Please note, however, to not delete direct beam work-spaces for efficient re-use for further reductions.
The information transfer between direct beam and reflected beam work-spaces is realized via sample log information exchange, see :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>` for more information.

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
