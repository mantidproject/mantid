.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

``LoadEventNexusAsHistogram`` reads events from each ``NXevent_data`` group and bins them directly into a
:ref:`Workspace2D <Workspace2D>`. It does not construct an intermediate :ref:`EventWorkspace <EventWorkspace>`.
This is useful when the required output binning is known in advance and event-level operations are not required.

Event IDs are converted to zero-based workspace indices by subtracting ``EventIDOffset``. The default offset of zero
matches MNeuEventLib files, in which event ID 0 represents the first spectrum. Output spectrum and detector numbers
begin at 1.

``Params`` accepts exactly three values: the first bin edge, a positive linear bin width, and the final bin edge, all
in microseconds. The units attribute on ``event_time_offset`` is used to convert nanoseconds, microseconds,
milliseconds, or seconds to microseconds before binning.

When ``pulse_height`` is present, an event is retained only if its value is strictly greater than
``MinimumPulseHeight``. This reproduces MNeuEventLib's default amplitude filter. Event IDs and times outside the
requested output range are ignored. Version 1 supports single-period files only.

Performance tuning
------------------

By default, ``ProcessingStrategy=Auto`` overlaps serial NeXus reads with histogramming of earlier chunks when the
input contains multiple chunks and more than one worker is available. HDF5 access remains serial; only in-memory
histogramming is parallel. This makes the pipeline independent of HDF5 thread-safety and uses Mantid's cross-platform
oneTBB runtime. ``ParallelEvents`` instead finishes each read before dividing that chunk's events between workers.
``Pipeline`` forces the pipelined path.

``MaxInFlightChunks`` limits pipeline memory usage. Its default of zero selects a limit from available concurrency
and a 256 MiB chunk-buffer budget. Set a positive value to tune throughput and memory use for a particular system.
``ReadSizeFromDisk`` controls the number of events in each chunk; larger chunks can reduce HDF5 call overhead but use
more memory and provide fewer independent pipeline tasks.

The loader reads ``event_time_offset`` into its on-disk numeric type. ``TimeBinningStrategy=Auto`` uses integer
arithmetic in the file's native units when the dataset contains unsigned integers and every value in ``Params`` can
be represented exactly in those units. It otherwise uses double-precision microseconds. ``Native`` and ``Double``
force either path; ``Native`` reports an error when exact native-unit binning is unavailable.

``AccumulatorType=Auto`` uses 32-bit thread-local histogram counters only when the total number of input events is no
greater than the largest 32-bit count, which proves that no individual bin can overflow. It uses 64-bit counters for
larger inputs. The explicit ``UInt32`` and ``UInt64`` settings are available for controlled comparisons; ``UInt32``
is rejected when the total input event count makes it unsafe.

Usage
-----

.. code-block:: python

   output = LoadEventNexusAsHistogram(
       Filename="HIFI00207031.nxs",
       NumberOfSpectra=64,
       Params="0,0.016,32.768",
       EventIDOffset=0,
       ProcessingStrategy="Auto",
       TimeBinningStrategy="Auto",
       AccumulatorType="Auto",
   )

.. categories::

.. sourcelink::
