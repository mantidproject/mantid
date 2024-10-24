.. _02_progress_reporting:

==================
Progress Reporting
==================

For long running algorithms it is often desirable to provide a mechanism of
reporting progress to the user. In MantidWorkbench, this is done via the
progress bar within the box that lists the available algorithms.

To add this mechanism to a Python algorithm you use the ``Progress`` class,
e.g.

.. code-block:: python

    def PyExec(self):
        endrange = 100000

        # Create a Progress object that covers the progress
        # of the whole algorithm (start=0.0,end=1.0) and will
        # report a maximum of endrange times
        prog_reporter = Progress(self, start=0.0, end=1.0,
                                 nreports=endrange)
        for i in range(0, endrange):
            prog_reporter.report("Processing")

Each time the ``report`` is called it can be passed an optional message and
this will update the progress bar in MantidWorkbench.

By default, the progress is incremented by a single unit when ``report`` is
called. The current value can be set using a variant of report, e.g.

.. code-block:: python

    def PyExec(self):
        endrange = 100000

        # Create a Progress object that covers the progress
        # of the whole algorithm (start=0.0, end=1.0)
        # and will report a maximum of endrange times
        prog_reporter = Progress(self, start=0.0, end=1.0,
                        nreports=endrange)
        for i in range(0, endrange/2):
            prog_reporter.report("Processing half")

        # Move progress to end
        prog_reporter.report(endrange, "Done")

It is also possible to step the progress in different increments using
``reportIncrement``, e.g. to step the reporting up by 5 "units":

.. code-block:: python

    def PyExec(self):
        endrange = 100000

        # Create a Progress object that covers the progress
        # of the whole algorithm (start=0.0,end=1.0)
        # and will report a maximum of endrange times
        prog_reporter = Progress(self, start=0.0, end=1.0,
                        nreports=endrange)
        for i in range(0, endrange):
            if i % 5 == 0:
                prog_reporter.reportIncrement(5, "Processing")

Cancellation
============

MantidWorkbench allows a cancellation request to be sent to a running
algorithm. An algorithm must be coded to stop itself if a request has been
made. This comes for **free** if you implement progress reporting as each
report call checks whether it should be be cancelled and stops if it should.
