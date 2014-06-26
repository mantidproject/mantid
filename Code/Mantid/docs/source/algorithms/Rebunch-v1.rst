.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm rebins data by adding together NBunch successive bins.

Usage
-----

.. testcode::

  # Create a workspaces
  ws = CreateSampleWorkspace()

  # Rebunch it
  rebunched = Rebunch( ws, 3 )

  # Check the result
  print 'Input workspace has      ',ws.blocksize(),'bins'
  print '"Rebunched" workspace has',rebunched.blocksize(),'bins'
  print 'Input workspace bin width      ',ws.readX(0)[1] - ws.readX(0)[0]
  print '"Rebunched" workspace bin width',rebunched.readX(0)[1] - rebunched.readX(0)[0]
  print 'Input workspace values      ',ws.readY(0)[0],ws.readY(0)[50],ws.readY(0)[70]
  print '"Rebunched" workspace values',rebunched.readY(0)[0],rebunched.readY(0)[50/3],rebunched.readY(0)[70/3]

Output
######

.. testoutput::

  Input workspace has       100 bins
  "Rebunched" workspace has 34 bins
  Input workspace bin width       200.0
  "Rebunched" workspace bin width 600.0
  Input workspace values       0.3 10.3 0.3
  "Rebunched" workspace values 0.9 10.9 0.9

.. categories::
