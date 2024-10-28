
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Load the events from the ``bank_error_events`` entry in the NeXus file. All events will be loaded, regardless of detector ID, into a single event spectrum.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - LoadErrorEventsNexus**

.. testcode:: LoadErrorEventsNexusExample

   ws = LoadErrorEventsNexus("REF_L_183110.nxs.h5")
   print(f"The output workspace has {ws.getNumberEvents()} events in {ws.getNumberHistograms()} spectra")

Output:

.. testoutput:: LoadErrorEventsNexusExample

  The output workspace has 82980 events in 1 spectra

.. categories::

.. sourcelink::
