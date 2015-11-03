
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows the user to set the MDFrame type for each axis on a workspace. 
The algorithm makes it possible to introduce the correct MDFrame type to 
workspaces which are associated with legacy files. Note that this can only be applied 
to MDHisto and MDEvent workspaces.

The selectable frame types are:
- "QLab" for a QLab-based frame
- "QSample" for a QSample-based frame
- "HKL" for an HKL-based frame
- "General Frame" for a general purpose frame
- "Unknown Frame" when the frame type is unknown

One frame-type needs to be selected for each dimension. Note that the default is set to a
general purpose frame. 

Usage
-----

**Example - Sample useage of SetMDFrames**

.. testcode:: SetMDFramesExample

    # Prepare the sample workspace
    mdws = CreateMDWorkspace(Dimensions=3, Extents='-10,10,-10,10,-10,10', 
                                        Names='A,B,C', Units='U,U,U', 
                                        Frames='QSample, QSample, QSample')
    print "The first MDFrame is of type " + mdws.getDimension(0).getMDFrame().name()
    print "The second MDFrame is of type " + mdws.getDimension(1).getMDFrame().name()
    print "The third MDFrame is of type " + mdws.getDimension(2).getMDFrame().name()

    # Set the MDFrames to new values
    SetMDFrames(mdws,MDFrame0='HKL', MDFrame1='HKL', MDFrame2='HKL')

    print "The new first MDFrame is of type " + mdws.getDimension(0).getMDFrame().name()
    print "The new second MDFrame is of type " + mdws.getDimension(1).getMDFrame().name()
    print "The new third MDFrame is of type " + mdws.getDimension(2).getMDFrame().name()

Output:

.. testoutput:: SetMDFramesExample

    The first MDFrame is of type QSample
    The second MDFrame is of type QSample
    The third MDFrame is of type QSample
    The new first MDFrame is of type HKL
    The new second MDFrame is of type HKL
    The new third MDFrame is of type HKL

.. categories::

.. sourcelink::

