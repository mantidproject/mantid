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

	def bank_size(bank_name):
		''' width and height of a rectangular bank, including any "scalex"/"scaley" resize parameters '''
		index = component_info.indexOfAny(bank_name)
		width = component_info.pixelGridNX(index) * component_info.pixelGridXStep(index)
		height = component_info.pixelGridNY(index) * component_info.pixelGridYStep(index)
		if component_info.hasParameter("scalex", index):
			width *= component_info.getNumberParameter("scalex", index)[0]
		if component_info.hasParameter("scaley", index):
			height *= component_info.getNumberParameter("scaley", index)[0]
		return width, height

	bank1_width, bank1_height = bank_size('bank1')
	bank2_width, bank2_height = bank_size('bank2')

	print ("bank 1 was scaled and is now {:.2f} by {:.2f}".format(bank1_width, bank1_height))
	print ("bank 2 was not scaled and remains {:.2f} by {:.2f}".format(bank2_width, bank2_height))

Output:

.. testoutput:: ExScaleBank1

	bank 1 was scaled and is now 0.16 by 0.04
	bank 2 was not scaled and remains 0.08 by 0.08

.. categories::

.. sourcelink::
