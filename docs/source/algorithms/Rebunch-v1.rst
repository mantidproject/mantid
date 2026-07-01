.. algorithm::

.. summary::

.. relatedalgorithms::

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
  print('Input workspace has             {} bins'.format(ws.blocksize()))
  print('"Rebunched" workspace has       {} bins'.format(rebunched.blocksize()))
  print('Input workspace bin width       {:.1f}'.format(ws.x(0)[1] - ws.x(0)[0]))
  print('"Rebunched" workspace bin width {:.1f}'.format(rebunched.x(0)[1] - rebunched.x(0)[0]))
  print('Input workspace values          {:.1f} {:.1f} {:.1f}'.format(ws.y(0)[0], ws.y(0)[50], ws.y(0)[70]))
  print('"Rebunched" workspace values    {:.1f} {:.1f} {:.1f}'.format(rebunched.y(0)[0], rebunched.y(0)[50//3], rebunched.y(0)[70//3]))

Output
######

.. testoutput::

  Input workspace has             100 bins
  "Rebunched" workspace has       34 bins
  Input workspace bin width       200.0
  "Rebunched" workspace bin width 600.0
  Input workspace values          0.3 10.3 0.3
  "Rebunched" workspace values    0.9 10.9 0.9

.. categories::

.. sourcelink::
