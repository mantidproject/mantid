.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the reduction work-flow algorithm for the ILL reflectometers D17 and FIGARO.
A various number of corresponding direct and reflected angles can be processed.
Some input properties like the background indices accept either a single entry for all angles, i.e. number of reflected or direct beams, or an entry for each corresponding angle.
Please note, however, that direct beam work-spaces for each angle are stored in the AnalysisDataService for efficient re-use of further reductions.
The input runs must be NeXus files (file ending .nxs) and different angles will be comma separated while merging angles will be performed when giving a *+*.

Limitations
-----------

The polarisation correction is foreseen but not exposed to be used (missing reflected beam inputs).
The D17 incoherent reduction with Sample Angle option will most likely produce equivalent data to Cosmos.
The D17 incoherent reduction with Detector Angle option may show a shift in Momentum Transfer.
The D17 coherent reduction often produces equivalent data to Cosmos, but rarely a shift in Momentum Transfer can be observed and need still to be investigated.
The detector angle option is more critical to use than the sample angle option.
It likely happens that the reduced output work-space does not have the same number of points as the Cosmos reduction result.
For FIGARO, a good agreement of the reduction could be observed, but not enough experiments were compared.
Not all options of the Cosmos GUI are available. For example, a modification of variables taken from the NeXus file should be done via the available instrument control tools.
Do not use the workbench, prefer Python scripts or MantidPlot: if investigating work-spaces (history, plots, ...), it may crash.

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
