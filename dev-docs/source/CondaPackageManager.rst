.. _CondaPackageManager:

=====================
Conda Package Manager
=====================

.. contents::
   :local:

Mantid uses `Conda <https://docs.conda.io/en/latest/>`_ as its package management system. This document gives a
developer overview on how we use the Conda package manager, including tips on how to debug dependency issues, and
our policy towards using pip packages (it is strongly discouraged).

Useful Conda Guides
-------------------

Tips for finding a broken dependency
------------------------------------

Fixing a dependency issue
-------------------------

After identifying the Conda dependency and version which is causing the unwanted behaviour, there are several
options we can take to fix the issue. The following options are in order of preference:

1. Raise an issue in the dependencies feedstock repository with a minimum reproducible example. If appropriate,
   request that they mark the package version as "Broken". See `Removing broken packages <https://conda-forge.org/docs/maintainer/updating_pkgs.html#maint-fix-broken-packages>`_ to understand this procedure.

2. If we need a fix urgently, you can consider pinning the package in question. This is not an ideal solution,
   and so you should also open an issue to un-pin the package in future. When pinning a package, consider
   using the not-equals-to operator ``!=x.y.z`` because this allows the package to upgrade automatically when
   a new version arrives (which is hopefully a working version).

Pip package policy
------------------


Profiling with Valgrind
-----------------------

This option is Linux only. See :doc:`ProfilingWithValgrind` for details.

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

*Note: This will not with QThreads and may crash or be unable to profile inside. It is recommended you migrate workers by inheriting from IQtAsync, see async_qt_adaptor.py for details.  Using native threading enables tooling, makes testing simple and produces readable stack traces.*

To use Yappi instead of cProfile simple append `--yappi` to the list of arguments after profile, e.g.:

.. code:: shell

   # If you do not have yappi installed
   python3 -m pip install yappi --user
   # Run with Yappi profiler
   ./MantidWorkbench --profile name_of_profile.out --yappi

KCachegrind can be used to view profiling data, see :doc:`ProfilingWithValgrind` for more details on usage.


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
