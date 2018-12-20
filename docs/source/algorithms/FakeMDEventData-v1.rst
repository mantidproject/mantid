.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

For testing `MDEventWorkspaces <http://www.mantidproject.org/MDEventWorkspace>`_,
this algorithm either creates a uniform, random distribution of events, or generates
regular events placed in boxes, or fills peaks around given points with a given
number of events.

Usage
-----

This algorithm can be run on a pre-existing `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_
or a newly created one. All of the examples below will be done with newly created ones
using :ref:`CreateMDWorkspace <algm-CreateMDWorkspace>`.

**2D Uniform Event Distribution Example**

.. testcode:: Uniform2dEx

    ws = CreateMDWorkspace(Dimensions='2', EventType='MDEvent', Extents='-10,10,-10,10',
                           Names='Q_lab_x,Q_lab_y', Units='A,B')
    FakeMDEventData(ws, UniformParams="1000000")
    print("Number of events = {}".format(ws.getNEvents()))

Output:

.. testoutput:: Uniform2dEx

    Number of events = 1000000

The output looks like the following in the `SliceViewer <http://www.mantidproject.org/MantidPlot:_SliceViewer>`_:

.. image:: /images/FakeMDEventData_Uniform2D.png
    :alt: Uniform 2D FakeMDEventData

|

**3D Peaks Example**

Creates 3 peaks in (H,K,L) at (0,0,0), (1,1,0) and (1,1,1).

.. testcode:: Peaks3dEx

    ws = CreateMDWorkspace(Dimensions='3', Extents='-3,3,-3,3,-3,3', Names='h,k,l',
                           Units='rlu,rlu,rlu', SplitInto='4')
    FakeMDEventData(ws, PeakParams='10000,1,1,1,0.1', RandomSeed='63759', RandomizeSignal='1')
    FakeMDEventData(ws, PeakParams='100000,0,0,0,0.1', RandomSeed='63759', RandomizeSignal='1')
    FakeMDEventData(ws, PeakParams='40000,1,1,0,0.1', RandomSeed='63759', RandomizeSignal='1')
    print("Number of events = {}".format(ws.getNEvents()))

Output:

.. testoutput:: Peaks3dEx

    Number of events = 150000

The output looks like the following in the `SliceViewer <http://www.mantidproject.org/MantidPlot:_SliceViewer>`_:

.. image:: /images/FakeMDEventData_Peaks3D.png
    :alt: Peaks 3D FakeMDEventData

Running :ref:`BinMD <algm-BinMD>`
on the resulting workspace, the output looks like the following in the `VSI <http://www.mantidproject.org/VatesSimpleInterface_v2>`_:

.. image:: /images/FakeMDEventData_Peaks3D_VSI.png
    :alt: Peaks 3D FakeMDEventData in VSI
    :scale: 75%

|

**4D Peaks Example**

Creates a peak at (H,K,L) of (0,0,0) around T=5K.

.. testcode:: Peaks4dEx

    ws = CreateMDWorkspace(Dimensions='4', Extents='-1,1,-1,1,-1,1,0,10', Names='H,K,L,T', Units='rlu,rlu,rlu,K',
                           SplitInto='2', SplitThreshold='50')
    FakeMDEventData(ws, PeakParams='1e+06,0,0,0,5,0.2', RandomSeed='3873875')
    print("Number of events = {}".format(ws.getNEvents()))

Output:

.. testoutput:: Peaks4dEx

    Number of events = 1000000

The output looks like the following in the `SliceViewer <http://www.mantidproject.org/MantidPlot:_SliceViewer>`_ when
the T slider is moved to ~5K:

.. image:: /images/FakeMDEventData_Peaks4D.png
    :alt: Peaks 4D FakeMDEventData

Running :ref:`BinMD <algm-BinMD>`
on the resulting workspace to create a 3D workspace with L integrated, the output looks like the following in the
`VSI <http://www.mantidproject.org/VatesSimpleInterface_v2>`_:

.. image:: /images/FakeMDEventData_Peaks4D_as_3D_VSI.png
    :alt: Peaks 4D FakeMDEventData as 3D in VSI
    :scale: 75%

|

.. categories::

.. sourcelink::
