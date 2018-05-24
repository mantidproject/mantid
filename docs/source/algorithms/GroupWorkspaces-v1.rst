.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes two or more workspaces as input and creates an
output workspace group.

Usage
-----

.. testcode::

  # Create two workspaces
  ws1 = CreateSampleWorkspace()
  ws2 = CreateSampleWorkspace()

  # Group them
  group = GroupWorkspaces( [ws1,ws2] )

  # Check the result
  print("Workspace's type is {}".format(type(group)))
  print('It has {} entries'.format(group.getNumberOfEntries()))
  print('Its first  item is {}'.format(group.getItem(0)))
  print('Its second item is {}'.format(group.getItem(1)))

Output
######

.. testoutput::

  Workspace's type is <class 'mantid.api._api.WorkspaceGroup'>
  It has 2 entries
  Its first  item is ws1
  Its second item is ws2

.. categories::

.. sourcelink::
