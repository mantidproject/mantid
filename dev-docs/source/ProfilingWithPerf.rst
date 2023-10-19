Profiling with perf
===================

Perf is a tool designed for use with the linux kernel, but can be used to profile user apps as well.
It is available on all linuxes, but requries root permission to enable.
Much of this information was originally gained from `Nick Tompson's Performance Tuning Tutorail <https://github.com/NAThompson/performance_tuning_tutorial>`_ held at Oak Ridge National Laboratory.

.. note::

   perf is a sampling based performance tool.
   This means the results are percentages rather than absolute times.
   However, many visualizations will associate times as well.
   Disk access issues are almost completely invisible to perf-based tools.

Install and configure
---------------------

To install perf on ubuntu one needs

.. code-block:: sh

   sudo apt install linux-tools-common
   sudo apt install linux-tools-generic
   sudo apt install linux-tools-`uname -r`

the last command gets the kernel modules specific to your kernel.

The final step of configuration allows for getting more information from perf traces.
Any debug symbols that are found will aid in understanding the output.

.. code-block:: sh

   #!/bin/bash
   # Taken from Milian Wolf's talk "Linux perf for Qt developers"
   sudo mount -o remount,mode=755 /sys/kernel/debug
   sudo mount -o remount,mode=755 /sys/kernel/debug/tracing
   echo "0" | sudo tee /proc/sys/kernel/kptr_restrict
   echo "-1" | sudo tee /proc/sys/kernel/perf_event_paranoid
   sudo chown `whoami` /sys/kernel/debug/tracing/uprobe_events
   sudo chmod a+rw /sys/kernel/debug/tracing/uprobe_events

Running perf
------------

To profile a single test (this starts with ``time`` to see how long the overall test takes)

.. code-block:: sh

   time perf record -g ./bin/AlgorithmsTest FilterEventsTest

The report can be viewed in a couple of ways. Using the curses-based tool

.. code-block:: sh

   perf report --no-children -s dso,sym,srcline

The report can also be viewed `FlameGraph <https://github.com/brendangregg/FlameGraph>`_ which generates an ``.svg`` that can be viewed in a web browser

.. code-block:: sh

   perf script | ~/code/FlameGraph/stackcollapse-perf.pl | ~/code/FlameGraph/flamegraph.pl > flame.svg
