.. _ProfilingOverview:

==================
Profiling Overview
==================

.. contents::
   :local:

Profiling can mean a few different things, so here are the current ways!

Profiling with Valgrind
-----------------------

This option is Linux only. See :doc:`ProfilingWithValgrind` for details.

Profiling with cProfile
-----------------------

Mantid supports profiling of its underlying code using the `cProfile <https://docs.python.org/3/library/profile.html>`_ module from python.

Launch Mantid Workbench from the command line using the script in the installation folder, adding the ``--profile`` modifier with a path to the output file.

E.g: ``path/to/install/bin/workbench --profile path/to/outputFile.prof``

This profile can then either be sent to the developers upon request or be analysed using the python module `snakeviz <https://pypi.org/project/snakeviz/>`_.


Profiling an algorithm
----------------------

On Linux, the build can be configured to generate algorithm profiling information. See :doc:`AlgorithmProfiler <AlgorithmProfiler>` for details.


Other Profiling Tools
---------------------

.. _linux-1:

Linux
#####

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
