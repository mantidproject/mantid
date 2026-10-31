.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will resize a
:py:obj:`~mantid.geometry.RectangularDetector` by applying X and Y
scaling factors. Each pixel's position will be modified relative to the
0,0 point of the detector by these factors. Typically, a
RectangularDetector is constructed around its center, so this would
scale the detector around its center.

This only works on :py:obj:`~mantid.geometry.RectangularDetector`. Banks
formed by e.g. tubes cannot be scaled in this way.

Internally, this sets the "scalex" and "scaley" parameters on the
:py:obj:`~mantid.geometry.RectangularDetector`. Note that the scaling is
relative to the original size, and is not cumulative: that is, if you
Resize \* 2 and again \* 3, your final detector is 3 times larger than
the original, not 6 times.

Note: As of this writing, the algorithm does NOT modify the shape of
individual pixels. This means that algorithms based on solid angle
calculations might be off. Ray-tracing (e.g. peak finding) are
unaffected.

.. seealso:: :ref:`algm-MoveInstrumentComponent` and
             :ref:`algm-RotateInstrumentComponent` for other ways
             to move components.

Usage
-----

**Example - Resize bank 1:**

.. testcode:: ExScaleBank1

	# a sample workspace with rectangular detectors
	ws = CreateSampleWorkspace()

	ResizeRectangularDetector(ws,"bank1",2.0,0.5)

	component_info = ws.componentInfo()
	bank1_index = component_info.indexOfAny("bank1")
	bank2_index = component_info.indexOfAny("bank2")

	bank1_width = component_info.pixelGridXSize(bank1_index)
	bank1_height = component_info.pixelGridYSize(bank1_index)
	bank2_width = component_info.pixelGridXSize(bank2_index)
	bank2_height = component_info.pixelGridYSize(bank2_index)

	print ("bank 1 was scaled and is now {:.2f} by {:.2f}".format(bank1_width, bank1_height))
	print ("bank 2 was not scaled and remains {:.2f} by {:.2f}".format(bank2_width, bank2_height))

Output:

.. testoutput:: ExScaleBank1

	bank 1 was scaled and is now 0.16 by 0.04
	bank 2 was not scaled and remains 0.08 by 0.08

.. categories::

.. sourcelink::
