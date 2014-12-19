
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm calculates a normalization MD workspace for single crystal direct geometry inelastic experiments. 
Trajectories of each detector in reciprocal space are calculated, and the flux is integrated between intersections with each
MDBox.

.. Note::

    This is an experimental algorithm in Release 3.3. Please check the nightly Mantid build, and the Mantid webpage
    for better offline help and usage examples.

.. Note::

    If the MDEvent input workspace is generated from an event workspace, the algorithm gives the correct normalization
    only if the event workspace is cropped and binned to the same energy transfer range. If the workspace is not cropped, 
    one might have events in places where the normalization is calculated to be 0.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - MDNormDirectSC**

.. testcode:: MDNormDirectSCExample


   
.. testoutput:: MDNormDirectSCExample 

    
    
    
.. categories::

