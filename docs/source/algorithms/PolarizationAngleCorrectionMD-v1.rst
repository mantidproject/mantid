.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

User inputs an in-plane polarization :math:`P_a` angle between -180 and 180 degrees,
and a precision (between 0 and 1).
Then for every MD event, a polarization angle correction is applied as below.

1. If the MDEvent is in Q-sample frame, convert it to Q-lab by
   :math:`Q_{lab} = R \times Q_{sample}`
   where :math:`R` is goniometer rotation matrix.


2. Calculate the horizontal plane angle between momentum transfer and direct beam
   :math:`\gamma = \tan^{-1}(Q_{lab,x}, Q_{lab,z})`

3. Calculate the Scharpf angle as
   :math:`\alpha = \gamma - P_a`
   , where :math:`P_a` is the polarization angle

4. Correction factor :math:`F` is equal to :math:`1 / \cos(2\alpha)` if :math:`|\cos(2\alpha)|`
   is greater than precision, else it is zero.

5. Apply correction to each MDEvent as


.. math::

    I &= I \times F

    Err^2 &= Err^2 \times F^2



Inputs
======

Input MDEventWorkspace must be in Q-lab or Q-sample. :math:`|Q|` is not allowed.

Usage
-----

**Example - Run Applying Polarization Angle Correction**

.. testcode:: ExPolarizationAngleCorrectionMDSimple

   we1 = CreateSampleWorkspace(WorkspaceType='Event',
                              Function='Flat background',
                              BankPixelWidth=1,
                              XUnit='DeltaE',
                              XMin=-10,
                              XMax=19,
                              BinWidth=0.5)
   AddSampleLog(Workspace=we1,LogName='Ei', LogText='20.', LogType='Number')
   MoveInstrumentComponent(Workspace=we1, ComponentName='bank1', X=3, Z=3, RelativePosition=False)
   MoveInstrumentComponent(Workspace=we1, ComponentName='bank2', X=-3, Z=-3, RelativePosition=False)
   SetGoniometer(Workspace=we1, Axis0='30,0,1,0,1')

   # Old way
   we1c = HyspecScharpfCorrection(InputWorkspace=we1,
                                  PolarizationAngle=-10,
                                  Precision=0.2)
   md1c_sample = ConvertToMD(InputWorkspace=we1c, QDimensions='Q3D', Q3DFrames='Q_sample')

   # new way
   md1sample = ConvertToMD(InputWorkspace=we1, QDimensions='Q3D')
   mdpacMDsample = PolarizationAngleCorrectionMD(InputWorkspace=md1sample, PolarizationAngle=-10, Precision=0.2)
   r = CompareMDWorkspaces(Workspace1=md1c_sample, Workspace2=mdpacMDsample, Tolerance=0.001, CheckEvents=True)
   print('Number of MDEvents: {} == {}'.format(md1c_sample.getNEvents(), mdpacMDsample.getNEvents()))
   print('MDEventWorkspaces are equal = {}.  Results: {}'.format(r.Equals, r.Result))

Output:

.. testoutput:: ExPolarizationAngleCorrectionMDSimple

   Number of MDEvents: 1972 == 1972
   MDEventWorkspaces are equal = True.  Results: Success!

.. categories::

.. sourcelink::
