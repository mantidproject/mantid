.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

ApplyInstrumentToPeaks will update the instrument contained within
peaks of a :ref:`PeaksWorkspace <PeaksWorkspace>`. It will keep the
same detector ID and TOF but recalculate the detector positions and
therefore the Q-vectors.

The issue this is trying to address is that the instrument in the
PeaksWorkspace can be different to the instrument within the peaks and
when the instrument of the PeaksWorkspace is modified (by
:ref:`MoveInstrumentComponent <algm-MoveInstrumentComponent>`,
:ref:`RotateInstrumentComponent <algm-RotateInstrumentComponent>`,
:ref:`LoadIsawDetCal <algm-LoadIsawDetCal>`, *etc*) then instrument in
the peaks is not updated.

If a workspace is provided to the ``InstrumentWorkspace`` property
then the instrument from that workspace will be used, otherwise the
instrument in the ``InputWorkspace`` will be used.

Usage
-----

**Example**

First create a peaks workspace with 3 peaks with ``L1 == L2 == 1``

.. testcode:: example

   ws = CreateSampleWorkspace(NumBanks=3, BankPixelWidth=1)
   MoveInstrumentComponent(ws, ComponentName='moderator', RelativePosition=False, X=0, Y=0, Z=-1)
   MoveInstrumentComponent(ws, ComponentName='bank1', RelativePosition=False, X=1, Y=0, Z=0) # θ=90, ϕ=0
   MoveInstrumentComponent(ws, ComponentName='bank2', RelativePosition=False, X=0, Y=1, Z=0) # θ=90, ϕ=90
   MoveInstrumentComponent(ws, ComponentName='bank3', RelativePosition=False, X=0.5, Y=0.5, Z=np.sqrt(2)/2) # θ=45, ϕ=45
   peaks = CreatePeaksWorkspace(ws, NumberOfPeaks=0)

   AddPeak(peaks, ws, TOF=10000, DetectorID=1) # θ=90, ϕ=0
   AddPeak(peaks, ws, TOF=5000, DetectorID=2) # θ=90, ϕ=90
   AddPeak(peaks, ws, TOF=1234, DetectorID=3) # θ=45, ϕ=45

   for n in range(peaks.getNumberPeaks()):
       print("DetID={DetID} TOF={TOF:.1f}μs λ={Wavelength:.4f}Å Qsample={QSample}".format(**peaks.row(n)))

.. testoutput:: example

   DetID=1 TOF=10000.0μs λ=19.7802Å Qsample=[-0.317651,0,0.317651]
   DetID=2 TOF=5000.0μs λ=9.8901Å Qsample=[0,-0.635301,0.635301]
   DetID=3 TOF=1234.0μs λ=2.4409Å Qsample=[-1.28708,-1.28708,0.753953]

Now move source and detectors so ``L1 == L2 == 2``.  When you look at
the peaks nothing has been updated.

.. testcode:: example

   MoveInstrumentComponent(peaks, ComponentName='moderator', RelativePosition=False, X=0, Y=0, Z=-2)
   MoveInstrumentComponent(peaks, ComponentName='bank1', RelativePosition=False, X=2, Y=0, Z=0)
   MoveInstrumentComponent(peaks, ComponentName='bank2', RelativePosition=False, X=0, Y=2, Z=0)
   MoveInstrumentComponent(peaks, ComponentName='bank3', RelativePosition=False, X=1, Y=1, Z=np.sqrt(2))

   for n in range(peaks.getNumberPeaks()):
       print("DetID={DetID} TOF={TOF:.1f}μs λ={Wavelength:.4f}Å Qsample={QSample}".format(**peaks.row(n)))

.. testoutput:: example

   DetID=1 TOF=10000.0μs λ=19.7802Å Qsample=[-0.317651,0,0.317651]
   DetID=2 TOF=5000.0μs λ=9.8901Å Qsample=[0,-0.635301,0.635301]
   DetID=3 TOF=1234.0μs λ=2.4409Å Qsample=[-1.28708,-1.28708,0.753953]

So we apply this algorithm and the peaks are updated, the wavelength
is halved and the q-vector doubled as expected while keeping the same
detector ID and TOF.

.. testcode:: example

   peaks = ApplyInstrumentToPeaks(peaks)

   for n in range(peaks.getNumberPeaks()):
       print("DetID={DetID} TOF={TOF:.1f}μs λ={Wavelength:.4f}Å Qsample={QSample}".format(**peaks.row(n)))

.. testoutput:: example

   DetID=1 TOF=10000.0μs λ=9.8901Å Qsample=[-0.635301,0,0.635301]
   DetID=2 TOF=5000.0μs λ=4.9450Å Qsample=[0,-1.2706,1.2706]
   DetID=3 TOF=1234.0μs λ=1.2204Å Qsample=[-2.57415,-2.57415,1.50791]

.. categories::

.. sourcelink::
