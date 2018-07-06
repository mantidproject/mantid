.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This can also be performed using the :ref:`algm-Rebin` algorithm and
having the "PreserveEvents" parameter set to false.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Simple conversion of an EventWorkspace**

.. testcode:: ConvertToMatrixWorkspaceSimpleExample

   event_ws = Load('CNCS_7860_event.nxs')
   # Run the conversion algorithm
   histo_ws = ConvertToMatrixWorkspace(event_ws)
   # Check that the original workspace and the converted workspace have the same shape
   print("{} {}".format(event_ws.getNumberHistograms(), event_ws.blocksize()))
   print("{} {}".format(histo_ws.getNumberHistograms(), histo_ws.blocksize()))

.. testoutput:: ConvertToMatrixWorkspaceSimpleExample

   51200 1
   51200 1

**Example - Conversion when blocksize > 1**

.. testcode:: ConvertToMatrixWorkspaceComplexExample

   event_ws = Load('CNCS_7860_event.nxs')
   # Rebin the loaded EventWorkspace to a new EventWorkspace that has > 1 bins
   event_ws_rebinned = Rebin(InputWorkspace=event_ws, Params=1e3, PreserveEvents=True)
   # Run the conversion algorithm
   histo_ws_rebinned = ConvertToMatrixWorkspace(event_ws_rebinned)
   # Check that the original workspace and the converted workspace have the same shape
   print("{} {}".format(event_ws_rebinned.getNumberHistograms(), event_ws_rebinned.blocksize()))
   print("{} {}".format(histo_ws_rebinned.getNumberHistograms(), histo_ws_rebinned.blocksize()))

Output:

.. testoutput:: ConvertToMatrixWorkspaceComplexExample    
  
   51200 17
   51200 17

.. categories::

.. sourcelink::
