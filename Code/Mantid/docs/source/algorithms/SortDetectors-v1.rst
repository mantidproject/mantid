.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm to sort detectors by distance. Will return arrays for upstream
(downstrem) spectrum number and detector distances, ordered by distance.

Usage
-----
.. testcode:: SortDetectors

   # create some workspace with an instrument
   ws = CreateSampleWorkspace()

   upIDs,upDistances,downIDs,downDistances=SortDetectors(ws)
   
   # No upstream detectors
   print "Type of upID:",type(upIDs)
   print "Number of upDistances:",upDistances.shape[0]
   #Downstream detectors
   print "First few values of downIDs: ", downIDs[0:5]
   print "First few values of downDistances: ", downDistances[0], downDistances[1], downDistances[2], downDistances[3],downDistances[4]

.. testcleanup:: SortDetectors

   DeleteWorkspace(ws)

Output:

.. testoutput:: SortDetectors

   Type of upID: <type 'numpy.ndarray'>
   Number of upDistances: 0
   First few values of downIDs:  [ 0  1 10 11  2]
   First few values of downDistances:  5.0 5.0000064 5.0000064 5.00001279998 5.00002559993
   
.. categories::
