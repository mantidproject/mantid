.. _03_instrument_view:

=======================
Instrument View Control
=======================

* When a workspace is loaded for a particular instrument, a virtual geometry is also created. The instrument view window within Mantid uses 3D rendering to visualize this geometry.

* Various aspects of this window are controllable from a Python script. This functionality is most useful for performing a set of operations before raising the window.

* For example, to set the minimum and maximum scales for the colour map before displaying the window

.. code-block:: python

	# First get a handle to the view for a given workspace
	instrument_view = getInstrumentView("workspace-name")

	# Then retrieve a handle to the render tab
	render = instrument_view.getTab(InstrumentWidget.RENDER)

	# Set the colour map range
	render.setMinValue(10.)
	render.setMaxValue(50.) 

	# Raise the window
	instrument_view.show()


Other characteristics that can be controlled from the script are

.. code-block:: python

	# Change the colour map
	import os
	render.changeColorMap(os.path.join(config["colormaps.directory"],"BlackBodyRadiation.MAP"))

	# Set the colour map min/max together
	render.setRange(10.,50.)
	# Alter the binning range
	instrument_view.setBinRange(10000,15000)

	# Select a component that the view should zoom to
	# (Note: This is not case sensitive). Remember to give a valid component name.
	tree_tab = instrument_view.getTab(InstrumentWidget.TREE)
	tree_tab.selectComponentByName("low angle bank")

The instrument can be also viewed without loading any data. The algorithm LoadEmptyInstrument loads the instrument definition into its own workspace that can be used to view the instrument

.. code-block:: python

	# Load the instrument into a workspace
	import os
	inesPath = os.path.join(ConfigService.getInstrumentDirectory(),"INES_Definition.xml")
	LoadEmptyInstrument(Filename=inesPath,
	                    OutputWorkspace="INES")
	view = getInstrumentView("INES")

	# Other operations as above
	# ... 
	# ...

	# Show the window
	view.show()

Helpful Hint
------------

.. code-block:: python

	print(config['instrumentDefinition.directory'])
	print(config['colormaps.directory'])

More documentation on InstrumentView python control