.. _03_live_data_user_interface:

=========================
 Live Data User Interface 
=========================

.. figure:: /images/LoadLiveDataButton.png
   :alt: LoadLiveDataButton.png

How to Start a live data stream
===============================

To start a live data stream into Mantid you need to use the
:ref:`algm-StartLiveData` algorithm. You can do this the same
way as any other algorithm using the Algorithms toolbox, or you can
select the Live Data option under the Load button.

Start Live Data
===============

The start live data interface can seem quite daunting at first, but it
is easy to do setup simple data streams quite easily, the next section
will take you through several examples.

.. figure:: /images/StartLiveData.png
   :alt: StartLiveData.png

For now I'll just introduce a few of the more common properties.

Starting Time
   This allows you to set when you want the stream to start at
   facilities that support this. If your facility does not support
   variable start times then the stream will start from "now".

Update Every
   Here you set the delay between each "chunk" of data being read. The
   optimal value depends on the amount of processing you are doing, but
   I suggest you start with something like 3 seconds.

Processing
   Here you select what processing you want to do to each live data
   chunk on it's way to the accumulation workspace. If you choose
   algorithm or script the Processing step pane on the right will change
   to allow you to define the processing.

Preserve Events
   This only applies to event based live data streams. If you tick
   preserve events the data will be maintained as events through to the
   accumulation workspace, this allows more flexibility for filtering
   and rebinning later, but will lead to large amounts or data
   accumulating in memory over time, eventually the system can run out
   of memory and the live data stream would stop.

Accumulation Method
   Here you can choose how the data is "added" into the accumulation
   workspace. In most cases for event based data the correct option is
   "Add", and for Histogram based data it is "Replace". There is also an
   "append" option which will append spectra to the output workspace,
   increasing its size.

Post Processing
   Here you select what post processing you want to do to the entire
   data in the accumulation workspace each time a new chunk is "added".
   If you choose algorithm or script the Post Processing step pane on
   the right will change to allow you to define the processing.

At End of Run
   At facilities that support this you can define what you want Mantid
   to do when one run is ended and another started.

Accumulation Workspace
   This is the name that will be used for the accumulation workspace. If
   no post-processing is selected this is greyed out as it is not
   needed.

Output Workspace
   The name of the output workspace containing the results you are
   after.

.. figure:: /images/RunningAlgDetailsButton.png
   :alt: RunningAlgDetailsButton.png

How to Stop a live data stream
==============================

Once you start a live data stream it creates a background algorithm
called MonitorLiveData which keeps the process running. This means it
will continue until one of the following happens:

#. MantidPlot (or Mantid if using through command line python) is
   closed.
#. You manually cancel the MonitorLiveData algorithm using the process
   below.
#. The run ends (although the process may continue if you set the live
   data stream to continue on run end).

To manually stop the live data stream you need to:

#. Bring up the running algorithm details using the "Details" button at
   the bottom of the algorithms toolbox (next to the progress bar).
#. Click the cancel button next to the MonitorLiveData algorithm you
   wish to stop.
#. If you have more than one, clicking the arrow next to the algorithm
   name will list all of the properties to help you select the right one
   to cancel.

|RunningAlgProgressDialog.png|

.. |RunningAlgProgressDialog.png| image:: /images/RunningAlgProgressDialog.png
