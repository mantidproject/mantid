.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Clears the OrientedLattice of each ExperimentInfo attached to the intput
`Workspace <http://www.mantidproject.org/Workspace>`_. Works with both single ExperimentInfos and
MultipleExperimentInfo instances.

Usage 
-----

.. testcode:: ExClearUB

   # create a workspace (or you can load one)
   ws=CreateSingleValuedWorkspace(5)

   #set a UB matrix using the vector along k_i as 1,1,0, and the 0,0,1 vector in the horizontal plane
   SetUB(ws,a=5,b=6,c=7,alpha=90, beta=90, gamma=90, u="1,1,0", v="0,0,1")

   #check that we do have a UB matrix
   from numpy import *
   mat=array(ws.sample().getOrientedLattice().getUB())
   print "UB matrix size", mat.size 

   ClearUB(ws)

   #check that it removed UB matrix & orientated lattice
   if( ws.sample().hasOrientedLattice() ):
	print "ClearUB has not removed the orientated lattice."
   else:
	print "ClearUB has removed the oriented lattice."

Output:

.. testoutput:: ExClearUB

   UB matrix size 9
   ClearUB has removed the oriented lattice.

.. categories::
