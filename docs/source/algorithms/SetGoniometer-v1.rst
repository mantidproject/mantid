.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Use this algorithm to define your goniometer. Enter each axis in the
order of rotation, starting with the one farthest from the sample.

You may enter up to 6 axes, for which you must define (separated by
commas):

-  The name of the axis, which MUST match the name in your sample logs.
   You can specify a number, and a log value will be created
   (GoniometerAxis?\_FixedValue, where ? is the axis number)
-  The X, Y, Z components of the vector of the axis of rotation.
   Right-handed coordinates with +Z=beam direction; +Y=Vertically up
   (against gravity); +X to the left.
-  The sense of rotation as 1 or -1: 1 for counter-clockwise, -1 for
   clockwise rotation.

The run's sample logs will be used in order to determine the actual
angles of rotation: for example, if you have an axis called 'phi',
then the time-average value of the log called 'phi' will be used as
the rotation angle. Units are assumed to be degrees.

When the Average property is set to False then a separate goniometer
will be created for each value in the log. When multiple log axes are
defined they must have the same length and are assumed to be all the
same time series.

The "Universal" goniometer at SNS is equivalent to Axis0 tied to the
"omega" log pointing vertically upward, Axis1 tied to "chi" log,
pointing along the beam, and Axis2 tied to "phi", pointing vertically
upward.

SetGoniometer(w,"Universal") is the same as
SetGoniometer(w,Axis0="omega,0,1,0,1",Axis1="chi,0,0,1,1",Axis2="phi,0,1,0,1")



Usage
-----

.. testcode:: SetGoniometer

    wg = CreateSingleValuedWorkspace()
    AddSampleLog(wg, "Motor1", "45.", "Number")
    SetGoniometer(wg, Axis0="Motor1,0,1,0,1", Axis1="5,0,1,0,1")

    print("Log values: {}".format(wg.getRun().keys()))
    print("Goniometer angles:  {}".format(wg.getRun().getGoniometer().getEulerAngles('YZY')))
    print("Clearing goniometer up")
    SetGoniometer(wg)
    print("Goniometer angles:  {}".format(wg.getRun().getGoniometer().getEulerAngles('YZY')    ))

.. testcleanup:: SetGoniometer

    DeleteWorkspace('wg')

Output:

.. testoutput:: SetGoniometer

    Log values: ['Motor1', 'GoniometerAxis1_FixedValue']
    Goniometer angles:  [50,0,0]
    Clearing goniometer up
    Goniometer angles:  [0,0,0]

**Example - multiple goniometers - omega scan**

.. testcode:: OmegaScanExample

   ws = LoadILLDiffraction(Filename='ILL/D20/000017.nxs')
   SetGoniometer(ws, Axis0='omega.position,0,1,0,1', Average=False)

   print('Number of goniometers =', ws.run().getNumGoniometers())

   for i in range(ws.run().getNumGoniometers()):
       print(f'{i} omega = {ws.run().getGoniometer(i).getEulerAngles("YZY")[0]:.1f}')

Output:

.. testoutput:: OmegaScanExample

   Number of goniometers = 21
   0 omega = 1.0
   1 omega = 1.2
   2 omega = 1.4
   3 omega = 1.6
   4 omega = 1.8
   5 omega = 2.0
   6 omega = 2.2
   7 omega = 2.4
   8 omega = 2.6
   9 omega = 2.8
   10 omega = 3.0
   11 omega = 3.2
   12 omega = 3.4
   13 omega = 3.6
   14 omega = 3.8
   15 omega = 4.0
   16 omega = 4.2
   17 omega = 4.4
   18 omega = 4.6
   19 omega = 4.8
   20 omega = 5.0

**Example: WISH goniometer**

The WISH instrument at ISIS has a goniometer arm at 45 degrees to vertical (phi axis) that is closest to the sample, and
a vertical rotation axis furthest from the sample (omega axis). The rotation angles about these axes are stored in the
``ewald_pos`` and ``ccr_pos`` logs of the workspace. The initial orientation of the phi-axis (at omega=0) is typically
in the plane normal to the incident beam (i.e. the XY-plane). In this case the goniometer can be set as so

.. code-block:: python

    ws = LoadEmptyInstrument(InstrumentName='WISH')
    SetGoniometer(ws, Axis0="ccr_pos,0,1,0,1",Axis1="ewald_pos,1,1,0,1")


.. categories::

.. sourcelink::
