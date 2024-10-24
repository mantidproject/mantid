.. _ProfilingOverview:

==================
Profiling Overview
==================

.. contents::
   :local:

Profiling can mean a few different things, so here are the current ways!
Many of these work only on certain platforms and this document will note the limitations.
Tools that are noted as being supported in linux are likely to work in osx as well.
Most of these approaches are specific to a single language.

Profiling in Python
===================

Profiling with cProfile
-----------------------

Mantid supports profiling of its underlying code using the `cProfile <https://docs.python.org/3/library/profile.html>`_ module from python.

Launch Mantid Workbench from the command line using the script in the installation folder, adding the ``--profile`` modifier with a path to the output file.

E.g: ``path/to/install/bin/workbench --profile path/to/outputFile.prof``

This profile can then either be sent to the developers upon request or be analysed using the python module `snakeviz <https://pypi.org/project/snakeviz/>`_.


Profiling with Yappi
--------------------

cProfile can only profile the current thread, this makes it useful for diagnosing PyQt performance issues (such as random hangs when a user does something), but poor for diagnosing workflow algorithm issues.

`Yappi (Yet Another Python Profiler) <https://pypi.org/project/yappi/>`_ is multithreading aware. cProfile which will show everything up to the exec, or run call. Yappi will profile the thread and show any slow points within the separate thread.

*Note: This will not work with QThreads and may crash or be unable to profile inside. It is recommended you migrate workers by inheriting from IQtAsync, see ``async_qt_adaptor.py`` for details.
Using native threading enables tooling, makes testing simple and produces readable stack traces.*

To use Yappi instead of cProfile simple append `--yappi` to the list of arguments after profile, e.g.:

.. code:: shell

   # If you do not have yappi installed
   python3 -m pip install yappi --user
   # Run with Yappi profiler
   ./MantidWorkbench --profile name_of_profile.out --yappi

KCachegrind can be used to view profiling data, see :doc:`ProfilingWithValgrind` for more details on usage.

Profiling in C++
================

Mantid's Algorithm Profiler
---------------------------

On Linux, the build can be configured to generate algorithm profiling information. See :doc:`AlgorithmProfiler <AlgorithmProfiler>` for details.

Other Profiling Tools
---------------------

.. _linux-1:

Linux
#####

:doc:`ProfilingWithPerf` for details on perf and intel's vtune

:doc:`ProfilingWithValgrind` for details on using this tool

`Callgrind/KCachegrind <http://kcachegrind.sourceforge.net/cgi-bin/show.cgi/KcacheGrindIndex>`__

-  KCachegrind visualizes callgrind output.
-  See :ref:`Profiling With Valgrind <ProfilingWithValgrind>` for help on
   running callgrind

`gperftools <https://github.com/gperftools/gperftools>`__

-  Takes snapshot of run and prints percentage of calls in functions

See here for a list of other tools:
http://www.pixelbeat.org/programming/profiling/

.. _windows-1:

Windows
#######

`Very Sleepy <http://www.codersnotes.com/sleepy/>`__:

-  Start/stop recording of program using a button
-  Not as detailed or flexible as callgrind

Timing in C++
-------------

Please refer to :doc:`Mantid Timers <Timers>` for an introduction to measuring execution time of the Mantid C++ code.
