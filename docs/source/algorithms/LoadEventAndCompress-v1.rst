
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that loads an event nexus file in chunks
and compresses the resulting chunks before summing them. It uses the
algorithms:

#. :ref:`algm-DetermineChunking`
#. :ref:`algm-LoadEventNexus`
#. :ref:`algm-FilterBadPulses`
#. :ref:`algm-CompressEvents`
#. :ref:`algm-Plus` to accumulate


Workflow
########

.. diagram:: LoadEventAndCompress-v1_wkflw.dot


Usage
-----
**Example - LoadEventAndCompress**

The files needed for this example are not present in our standard usage data
download due to their size.  They can however be downloaded using these links:
`PG3_9830_event.nxs <https://github.com/mantidproject/systemtests/blob/master/Data/PG3_9830_event.nxs?raw=true>`_.


.. code-block:: python

   PG3_9830_event = LoadEventAndCompress(Filename='PG3_9830_event.nxs',
                                         MaxChunkSize=1.)

**Example - Usage with MPI**

Create a python driver script called test_mpi.py

.. code-block:: python

    from mantid.simpleapi import *
    import mantid
    if AlgorithmFactory.exists('GatherWorkspaces'):
        HAVE_MPI = True
        from mpi4py import MPI
        mpiRank = MPI.COMM_WORLD.Get_rank()
        mpiSize = MPI.COMM_WORLD.Get_size()
    else:
        HAVE_MPI = False
        mpiRank = 0 # simplify if clauses
        mpiSize = 1 # simplify if clauses

    wksp = LoadEventAndCompress(Filename="PG3_2538_event.nxs")
    print("Rank =  {} Number of Events =  {}".format(mpiRank, wksp.getNumberEvents()))
    if mpiRank == 0:
        reduce = AlignAndFocusPowder(InputWorkspace=wksp, CalFileName='PG3_calibrate_d2538_2014_05_13.cal', Params='0.5,0.01,2')
        SaveNexus(reduce,Filename=str(mpiSize)+"tasks.nxs")

And run it using the following commands

.. code-block:: bash

    $ module load mpi/openmpi-x86_64
    $ export LD_PRELOAD=/usr/lib64/openmpi/lib/libmpi.so
    $ mpirun -np 8 mantidpython test_mpi.py

to run without mpi is simply

.. code-block:: bash

    $ mantidpython test_mpi.py

.. categories::

.. sourcelink::
