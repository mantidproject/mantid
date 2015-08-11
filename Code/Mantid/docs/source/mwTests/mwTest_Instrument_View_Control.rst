:orphan:

.. testsetup:: mwTest_Instrument_View_Control[9]

   from mantidplot import *
   workspace_name = Load("MAR11015",OutputWorkspace="workspace-name")

.. testcode:: mwTest_Instrument_View_Control[9]

   # First get a handle to the view for a given workspace
   instrument_view = getInstrumentView("workspace-name")
   
   # Then retrieve a handle to the render tab
   render = instrument_view.getTab(InstrumentWindow.RENDER)
   
   # Set the colour map range
   render.setMinValue(10.)
   render.setMaxValue(50.) 
   
   # Raise the window
   instrument_view.show()


.. testsetup:: mwTest_Instrument_View_Control[30]

   from mantidplot import *
   workspace_name = Load("MAR11015",OutputWorkspace="workspace-name")
   # First get a handle to the view for a given workspace
   instrument_view = getInstrumentView("workspace-name")
   # Then retrieve a handle to the render tab
   render = instrument_view.getTab(InstrumentWindow.RENDER)

.. testcode:: mwTest_Instrument_View_Control[30]

   # Change the colour map
   import os
   render.changeColorMap(os.path.join(config["colormaps.directory"],"BlackBodyRadiation.MAP"))
   
   # Set the colour map min/max together
   render.setRange(10.,50.)
   # Alter the binning range
   instrument_view.setBinRange(10000,15000)
   
   # Select a component that the view should zoom to
   # (Note: This is not case sensitive). Remember to give a valid component name.
   tree_tab = instrument_view.getTab(InstrumentWindow.TREE)
   tree_tab.selectComponentByName("low angle bank")


.. testsetup:: mwTest_Instrument_View_Control[56]

   from mantidplot import *

.. testcode:: mwTest_Instrument_View_Control[56]

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


.. testcode:: mwTest_Instrument_View_Control[76]

   print config['instrumentDefinition.directory']
   print config['colormaps.directory']

.. testoutput:: mwTest_Instrument_View_Control[76]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ...


