.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Get Ei and T0 on ARCS and SEQUOIA instruments. It accounts for the
following:

-  in the ADARA framework, the monitors are in the first frame.
-  SEQUOIA has event based monitors.
-  some data acquisition errors will create unphysical monitor IDs. This
   will be ignored
-  when vChTrans is 2, on ARCS and SEQUOIA there is no chopper in the
   beam (white beam). Will return not a number for both Ei and T0
  
The algorithm is doing the following:

- Check which spectra corresponds to a physical monitor (as per instrument definition file)
- Change the time of flight in the monitor spectra to the correct frame
- Rebins the monitor workspace with a step of 1 microsecond
- Uses :ref:`GetEi <algm-GetEi>` to calculate incident energy and T0
   
Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: GetEiT0atSNS
    
    w=Load('ADARAMonitors.nxs')
    run.setStartAndEndTime(
        DateAndTime("2015-01-27T11:00:00"),
        DateAndTime("2015-01-27T11:57:51")
    )
    LoadInstrument(Workspace=w,InstrumentName='SEQUOIA',RewriteSpectraMap=False)
    LoadInstrument(Workspace=w,InstrumentName='SEQUOIA',RewriteSpectraMap=False)
    AddSampleLog(Workspace=w,LogName='vChTrans',LogText='1',LogType='Number Series')
    AddSampleLog(Workspace=w,LogName='EnergyRequest',LogText='20',LogType='Number Series')
    res=GetEiT0atSNS(w)    
    
    print("Incident energy: {:2.2f} meV".format(res[0]))
    print("T0: {:2.2f} microseconds".format(res[1]))


.. testcleanup:: GetEiT0atSNS

   DeleteWorkspace('w')


Output:

.. testoutput:: GetEiT0atSNS
   
    Incident energy: 20.09 meV
    T0: 30.42 microseconds


.. categories::

.. sourcelink::
