.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Determine if a workspace has a :ref:`UB matrix <Lattice>` on any of it's
samples. Returns True if one is found. Returns false if none can be found,
or if the workspace type is incompatible.

Usage
-----

**Example**

.. testcode:: ExHasUB

   # create histogram workspace
   ws=CreateSampleWorkspace()

   returnVal = HasUB(ws)
   print('Before SetUB does {} have a UB Matrix? {}'.format(ws,returnVal))

   SetUB(ws,1,1,1,90,90,90)
   returnVal = HasUB(ws)
   print('After SetUB does {} have a UB Matrix? {}'.format(ws,returnVal))

   #This python call to the workspace gives the same information
   returnVal = ws.sample().hasOrientedLattice()
   print('Using ws.sample().hasOrientedLattice() does {} have a UB Matrix? {}'.format(ws,returnVal))


Output:

.. testoutput:: ExHasUB

    Before SetUB does ws have a UB Matrix? False
    After SetUB does ws have a UB Matrix? True
    Using ws.sample().hasOrientedLattice() does ws have a UB Matrix? True




.. categories::

.. sourcelink::
