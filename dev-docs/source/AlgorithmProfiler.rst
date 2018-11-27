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

To build mantid for profiling run ``cmake`` with the additional option ``-DPROFILE_ALGORITHM_LINUX=ON``.
Built in such a way mantid creates a dump file ``algotimeregister.out`` in the running directory.
This file contains the time stamps for start and finish of executed algorithms with ~nanocecond
precision in a very simple text format.

Analysing tool
^^^^^^^^^^^^^^

The project is available here: https://github.com/nvaytet/mantid-profiler. It provides the nice graphical
tool to interpret the information from dumped file.

Windows development
^^^^^^^^^^^^^^^^^^^

Precise timers are different for Linux and Windows (chrono is not good enough), so we need to treat them
separately. The suggestion is either to modify files ``Framework/API/inc/MantidAPI/AlgoTimeRegister.h`` and
``Framework/API/src/AlgoTimeRegister.cpp`` in manner ``#ifdef __WIN32`` or create the specific files with
the ``AlgoTimeRegister`` class for Windows and then in ``Framework/API/CMakeLists.txt`` edit the chunk that
describes option PROFILE_ALGORITHM_LINUX combined with OS defied flags e.g., also in this case
``Framework/API/src/AlgorithmExecuteProfile.cpp`` needs to be modified or substituted.
``${CMAKE_SYSTEM_NAME} MATCHES "Windows"``. QueryPerformanceCounter supposed to be used on windows,
instead of clock_gettime.

