.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used for finding Goniometer angles when instrument
readings are basically unknown. The inputs are two PeaksWorkspaces
corresponding to sample orientations of the SAME crystal that differ
only in their phi rotation.

If the phi angles are known, this algorithm attempts to find the common
chi and omega rotations.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: exGoniometerAnglesFromPhiRotation 

   # Load Peaks found in SXD23767.raw 
   ws1 =Load(Filename='SXD23767.peaks')
   ws2 = ws1.clone()

   #Set orientated lattice
   ublist = [-0.06452276,  0.2478114,  -0.23742194, 0.29161678, -0.00914316, -0.12523779, 0.06958942, -0.1802702,  -0.14649001]
   SetUB(Workspace=ws1,UB=ublist)

   # Run Algorithm 
   result = GoniometerAnglesFromPhiRotation(ws1,ws2,Tolerance=0.5,MIND=0,MAXD=2)

   print "Chi: %.1f, Omega: %.1f, Indexed: %i, AvErrIndex: %.4f AvErrAll: %.4f" % (  result[0], result[1], result[2], result[3], result[4] )

Output:

.. testoutput:: exGoniometerAnglesFromPhiRotation

   Chi: 90.0, Omega: 90.0, Indexed: 300, AvErrIndex: 0.2114 AvErrAll: 0.2114

.. categories::
