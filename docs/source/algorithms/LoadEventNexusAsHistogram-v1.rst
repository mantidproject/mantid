.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

``LoadEventNexusAsHistogram`` reads events from each ``NXevent_data`` group and bins them directly into a
:py:obj:`Workspace2D <mantid.dataobjects.Workspace2D>`. It does not construct an intermediate
:py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>`.
This is useful when the required output binning is known in advance and event-level operations are not required.

By default, ``LoadInstrument=True`` loads the instrument and maps events using **physical detector IDs**.
``EventIDOffset`` must be zero. The spectrum count is derived from the instrument; a non-default
``NumberOfSpectra`` must match this count. The instrument can be loaded from embedded XML or through the existing
Mantid instrument-loading mechanism, controlled by ``LoadNexusInstrumentXML``. Each non-monitor detector gets its own
spectrum, numbered from 1 in the same order as :ref:`algm-LoadEventNexus`. Known monitors are excluded from output
spectra. Unknown event IDs raise an error instead of guessing an offset or silently discarding detector events.
Failure to load an instrument with detectors also raises an error. Other file-specific mapping conventions are not
inferred automatically. The algorithm does not perform diffraction focusing or calibration.

With ``LoadInstrument=False``, no instrument is loaded. Event IDs are converted to zero-based workspace indices by
subtracting ``EventIDOffset``. For example, use an offset of zero when event ID 0 represents the first spectrum, or
an offset of one when event ID 1 represents the first spectrum. Output spectrum and detector numbers start at 1;
``NumberOfSpectra`` sets the output size. Set these two properties to match the file's event-ID numbering.

Legacy ``isis_vms_compat`` mapping is not supported with ``LoadInstrument=True``. If the selected NeXus entry
contains this group, the algorithm raises an error before loading instrument metadata or events. Use
:ref:`algm-LoadEventNexus` when these detector associations are required. With ``LoadInstrument=False``, the group is
ignored without reading or validating its mapping; detector associations from the legacy mapping are not preserved.

``Params`` accepts exactly three values: the first bin edge, a positive linear bin width, and the final bin edge, all
in microseconds. The units attribute on ``event_time_offset`` is used to convert nanoseconds, microseconds,
milliseconds, or seconds to microseconds before binning.

With the default ``FilterByPulseHeight=True``, when ``pulse_height`` is present, an event is retained only if its
value is strictly greater than ``MinimumPulseHeight``.
``FilterByPulseHeight=False`` skips reading and validating that dataset and accepts events regardless of amplitude,
including zero, negative and non-finite pulse heights. Detector mapping and time-range selection still apply.
Here pulse height is an individual detector event's amplitude, not a source-pulse selection.
With ``LoadInstrument=False``, event IDs outside the requested spectrum range are ignored. Times outside the
requested histogram range are ignored regardless of instrument loading. Version 1 supports single-period files only.

Performance tuning
------------------

The default settings are suitable for most files. The algorithm automatically selects the binning method and a
safe histogram counter size. The following properties allow loading speed and memory use to be tuned without
changing the requested histogram:

* ``ProcessingStrategy`` controls how reading and histogramming are scheduled. ``Pipeline`` overlaps reading with
  processing earlier chunks. ``ParallelEvents`` reads a chunk before processing its events in parallel. ``Auto``
  selects a strategy based on the input size and available workers.
* ``ReadSizeFromDisk`` sets the maximum number of events read in each chunk. Larger chunks can reduce reading
  overhead but require more memory.
* ``MaxInFlightChunks`` limits the number of chunks held by the pipeline. Zero selects an automatic limit.
  A smaller positive value can reduce memory use; a larger value may improve throughput.

Disabling ``LoadInstrument`` or ``FilterByPulseHeight`` can reduce loading work, but changes detector associations
or event selection respectively. Use these options only when those changes are appropriate for your analysis.

Usage
-----

This example loads the POWGEN event file ``PG3_5226_event.nxs`` into 100-microsecond bins spanning 0 to 17000
microseconds. The default ``LoadInstrument=True`` preserves physical detector associations and determines the
number of output spectra. Keep the default ``LoadLogs=True`` for this historical dataset: its run date is needed
to select the correct instrument definition.

.. code-block:: python

   output = LoadEventNexusAsHistogram(
       Filename="PG3_5226_event.nxs",
       Params="0,100,17000",
   )

.. categories::

.. sourcelink::
