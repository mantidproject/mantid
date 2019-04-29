==============================
Work flows algorithm profiling
==============================

.. contents:: Contents
    :local:

Summary
^^^^^^^

Due to the need of investigation of algorithms performance issues, the proper method
is introduced. It consists two to parts: special mantid build and analytical tool.
Available for Linux only.

Mantid build
^^^^^^^^^^^^

To build mantid version with profiling functionality enabled run ``cmake`` with the additional option
``-DPROFILE_ALGORITHM_LINUX=ON``. Built in such a way mantid creates a dump file ``algotimeregister.out``
in the running directory. This file contains the time stamps for start and finish of executed algorithms with
~nanosecond precision in a very simple text format.

Analysing tool
^^^^^^^^^^^^^^

The project is available here: https://github.com/nvaytet/mantid-profiler. It provides the nice graphical
tool to interpret the information contained in the dumped file.

Windows development
^^^^^^^^^^^^^^^^^^^

Precise timers are different for Linux and Windows (chrono is not good enough), so we need to treat them
separately. The suggestion is either to modify files ``Framework/API/inc/MantidAPI/AlgoTimeRegister.h`` and
``Framework/API/src/AlgoTimeRegister.cpp`` by including ``#ifdef __WIN32``, or create the specific files with
the ``AlgoTimeRegister`` class defined for Windows.  In the second case an inclusion of OS specific files should be
done correctly in ``Framework/API/CMakeLists.txt`  using ``${CMAKE_SYSTEM_NAME} MATCHES "Windows"``,
``Framework/API/src/AlgorithmExecuteProfile.cpp`` needs to be modified or substituted according to the type of OS.
``QueryPerformanceCounter`` supposed to be used on windows, instead of ``clock_gettime``.

