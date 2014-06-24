.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The StartLiveData algorithm launches a background job that monitors and
processes live data.

The background algorithm started is
:ref:`algm-MonitorLiveData`, which simply calls
:ref:`algm-LoadLiveData` at a fixed interval.

For details on the way to specify the data processing steps, see:
`LoadLiveData <LoadLiveData#Description>`__.

Live Plots
##########

Once live data monitoring has started, you can open a plot in
MantidPlot. For example, you can right-click a workspace and choose
"Plot Spectra".

As the data is acquired, this plot updates automatically.

Another way to start plots is to use `python MantidPlot
commands <MantidPlot:_Help#Python_Scripting_in_MantidPlot>`__. The
StartLiveData algorithm returns after the first chunk of data has been
loaded and processed. This makes it simple to write a script that will
open a live plot. For example:

.. code-block:: python

    StartLiveData(UpdateEvery='1.0',Instrument='FakeEventDataListener',
      ProcessingAlgorithm='Rebin',ProcessingProperties='Params=10e3,1000,60e3;PreserveEvents=1',
      OutputWorkspace='live')
    plotSpectrum('live', [0,1])

Run Transition Behavior
#######################

-  When the experimenter starts and stops a run, the Live Data Listener
   receives this as a signal.
-  The *RunTransitionBehavior* property specifies what to do at these
   run transitions.

   -  Restart: the accumulated data (from the previous run if a run has
      just ended or from the time between runs a if a run has just
      started) is discarded as soon as the next chunk of data arrives.
   -  Stop: live data monitoring ends. It will have to be restarted
      manually.
   -  Rename: the previous workspaces are renamed, and monitoring
      continues with cleared ones. The run number, if found, is used to
      rename the old workspaces.

      -  There is a check for available memory before renaming; if there
         is not enough memory, the old data is discarded.

-  Note that LiveData continues monitoring even if outside of a run
   (i.e. before a run begins you will still receive live data).

Multiple Live Data Sessions
###########################

It is possible to have multiple live data sessions running at the same
time. Simply call StartLiveData more than once, but make sure to specify
unique names for the *OutputWorkspace*.

Please note that you may be limited in how much simultaneous processing
you can do by your available memory and CPUs.

.. Usage examples need the fake services to be available in a test facility
.. Ticket #9671

.. categories::
