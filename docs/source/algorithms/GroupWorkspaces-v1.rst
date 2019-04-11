.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes two or more workspaces as input and creates an
output workspace group. A list of workspaces and a glob pattern may
be specified together.

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

  wrkspc1 = CreateSampleWorkspace()
  wrkspc2 = CreateSampleWorkspace()
  anotherGroup = GroupWorkspaces(GlobExpression='wrkspc?')

  # Check the result
  print('It has {} entries'.format(anotherGroup.getNumberOfEntries()))
  print('Its first  item is {}'.format(anotherGroup.getItem(0)))
  print('Its second item is {}'.format(anotherGroup.getItem(1)))

Output
######

.. testoutput::

  Workspace's type is <class '_api.WorkspaceGroup'>
  It has 2 entries
  Its first  item is ws1
  Its second item is ws2
  It has 2 entries
  Its first  item is wrkspc1
  Its second item is wrkspc2

.. categories::

.. sourcelink::
