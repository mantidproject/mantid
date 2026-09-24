.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm changes the sample location for an events workspace.  If the InputWorkspace and OutputWorkspace are the same, the position is simply changed.  If the InputWorkspace and OutputWorkspace are different, the InputWorkspace is cloned then the clone's position is changed.  The former is faster, especially for large workspaces.

Usage
-----

**Example:**

.. testcode:: ExSetCrystalLocation

  events = Load('BSS_11841_event.nxs')
  compInfo = mtd['events'].componentInfo()
  print('Sample position before SetCrystalLocation: {}'.format(compInfo.samplePosition()))
  SetCrystalLocation(InputWorkspace=events, OutputWorkspace=events, NewX=0.1, NewY=0.1, NewZ=0.1)
  print('Sample position after SetCrystalLocation: {}'.format(mtd['events'].componentInfo().samplePosition()))

Output:

.. testoutput:: ExSetCrystalLocation

  Sample position before SetCrystalLocation: [0,0,0]
  Sample position after SetCrystalLocation: [0.1,0.1,0.1]

.. categories::

.. sourcelink::
