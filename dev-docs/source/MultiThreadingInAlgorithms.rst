============================
Multithreading in Algorithms
============================

Mantid uses `OpenMP <http://openmp.org/wp/about-openmp/>`__ to improve
performance within algorithms by parallelizing ``for`` loops. A tutorial
devoted to the technology can be found `here <https://computing.llnl.gov/tutorials/openMP/>`__.

Access to the OpenMP API is via a set of macros defined in
`MultiThreading.h <https://github.com/mantidproject/mantid/blob/master/Framework/Kernel/inc/MantidKernel/MultiThreaded.h>`__.
This accomplishes seamless fall-back to single-threaded behaviour for
compilers that don't have OpenMP available, as well as providing
protection against multithreading when non-thread-safe workspaces are in use.

The canonical way to use OpenMP in an algorithm loop (typically
one over the spectra in a workspace) is as follows:

.. code:: cpp

     PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
     for (int i = 0; i < numSpec; ++i)
     {
       PARALLEL_START_INTERUPT_REGION

       // .... algorithm code ....

       PARALLEL_END_INTERUPT_REGION
     }
     PARALLEL_CHECK_INTERUPT_REGION


The main work is in the first statement, which contains the
instruction invoking OpenMP, but only if the workspaces given are
thread-safe. Analogous macros are available for zero, 2 or 3 workspaces.
Any workspace that is accessed within the loop should be included. There
is then also a set of slightly verbose interrupt instructions, which
prevent exceptions escaping from a parallel region (which would
otherwise cause program termination) - this includes dealing with
algorithm cancellation.

Ensuring thread-safety
----------------------

The first rule is this: **Don't write to shared variables.** Or, if you
do, protect the write with PARALLEL\_CRITICAL or PARALLEL\_ATOMIC calls.
Note that a write to a workspace data spectrum selected by the loop
index is not typically a shared write (though see below).

One gotcha comes from the use of copy-on-write pointers to store the
workspace data. Here's an example:

.. code:: cpp

   // Get references to the x data
   const auto& xIn = inputWS->x(i);
   auto& xOut = outputWS->mutableX(i);

This can cause problems in the case where the input and output
workspaces are the same. Although the call to ``outputWS->x()`` to get a
reference to the output data may look innocuous, in the case where
different spectra are pointing to the same underlying data array this
call will cause the array to be copied, which will invalidate the
reference obtained to the input data in the previous line. The solution
is to make sure the non-const calls come before the const ones (in this
case by reversing the two lines).
