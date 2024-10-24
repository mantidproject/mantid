========================
Multithreading in Mantid
========================

C++
---

Mantid uses `OpenMP <http://openmp.org/wp/about-openmp/>`__ in C++ to improve
performance by parallelizing ``for`` loops. A tutorial devoted to the technology can be found
`here <https://hpc-tutorials.llnl.gov/openmp/>`__.

Access to the OpenMP API is via a set of macros defined in
`MultiThreaded.h <https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/MultiThreaded.h>`__.
This accomplishes seamless fall-back to single-threaded behaviour for
compilers that don't have OpenMP available, as well as providing
protection against multithreading when non-thread-safe workspaces are in use.

The recommended way to use OpenMP in an algorithm loop (typically
one over the spectra in a workspace) is as follows:

.. code:: cpp

     PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
     for (int i = 0; i < numSpec; ++i)
     {
       PARALLEL_START_INTERRUPT_REGION

       // .... algorithm code ....

       PARALLEL_END_INTERRUPT_REGION
     }
     PARALLEL_CHECK_INTERRUPT_REGION


The main work is in the first statement, which contains the
instruction invoking OpenMP, but only if the workspaces given are
thread-safe. Analogous macros are available for zero, 2 or 3 workspaces.
Any workspace that is accessed within the loop should be included.

There is then also a set of slightly verbose interrupt instructions, which
prevent exceptions escaping from a parallel region (which would
otherwise cause program termination) - this includes dealing with
algorithm cancellation.

If you need to use OpenMP in a way that is not covered by any of the specific macros in
`MultiThreaded.h <https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/MultiThreaded.h>`__
, you can make use of the general purpose ``PRAGMA_OMP`` macro. This is essentially the same as using ``#pragma omp``
directly. Ideally, this should only be used if what you want to do is not already covered by the other macros.

.. code:: cpp

    // Dynamic scheduling allows x to be split into chunks of size 1 processed by each thread,
    // but assigned to threads in no particular order.
    PRAGMA_OMP(parallel for schedule(dynamic, 1))
    for (int i = 0; i < x.size(); ++i) {
        doThing(x[i])
    }

Note: The set of ``INTERRUPT`` macros can only be used in Mantid algorithms. The rest can be used anywhere.

Ensuring thread-safety
######################

The first rule is this: **Don't write to shared variables.** Or, if you
do, protect the write with PARALLEL\_CRITICAL or PARALLEL\_ATOMIC calls.

.. code:: cpp

     // Can only be used on simple operations, uses atomic access from machine hardware.
     PARALLEL_ATOMIC
     a++

     // Can be used anywhere, but has a higher overhead.
     // Can be named if two critical sections can be accessed simultaneously.
     PARALLEL_CRITICAL("C1")
     if(a > 4) {
         b.update()
     }

Note that a write to a workspace data spectrum selected by the loop
index is not typically a shared write (though see below).

One gotcha comes from the use of copy-on-write pointers to store the
workspace data. Here's an example:

.. code:: cpp

   // Get references to the x data
   const auto& xIn = inputWS->x(i);
   auto& xOut = outputWS->mutableX(i);

This can cause problems in the case where the input and output
workspaces are the same. Although the call to ``outputWS->mutableX()`` to get a
reference to the output data may look innocuous, in the case where
different spectra are pointing to the same underlying data array this
call will cause the array to be copied, which will invalidate the
reference obtained to the input data in the previous line. The solution
is to make sure the non-const calls come before the const ones (in this
case by reversing the two lines).

Python
------

Tasks in python can be run outside of the main GUI thread by using the classes and functions defined in
`asynchronous.py <https://github.com/mantidproject/mantid/blob/main/qt/python/mantidqt/mantidqt/utils/asynchronous.py>`__

The simplest and most commonly used one is ``AsyncTask``:

.. code:: python

    self.worker = AsyncTask(self.to_be_run, (param_1, param_2),
                            error_cb=self._on_worker_error,
                            finished_cb=self._on_worker_success)
    self.worker.start()

For more OpenMP style multithreading, there is the functionality inside
`async_qt_adaptor.py <https://github.com/mantidproject/mantid/blob/main/qt/python/mantidqt/mantidqt/utils/async_qt_adaptor.py>`__

The methods you wish to run asynchronously must be inside a class that inherits from ``IQtAsync``.
You can then overwrite any of the relevant callbacks and annotate async methods with the ``@qt_async_task`` decorator.

.. code:: python

    class DoesAsyncThings(IQtAsync):
        def __init__(self):
            super().__init__()

        def finished_cb_slot(self) -> None:
            self.task_finished()

        @qt_async_task
        def do_async(self):
            self.do_task()

Note: These methods are only useful for stopping mantid from hanging while something else is processing.
Due to the nature of the Global Interpreter Lock (GIL), it is not possible to run concurrent threads in python.
For heavy lifting that would require multithreading you should use C++ instead.
