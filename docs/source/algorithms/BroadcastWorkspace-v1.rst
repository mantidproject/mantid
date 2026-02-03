.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

BroadcastWorkspace distributes a workspace from one MPI process (the broadcaster) to all other processes
in the MPI job. Only the broadcaster rank needs to provide an input workspace; all other ranks will
receive an identical copy as their output. The algorithm is optimized for large workspaces through
chunked broadcasting, which processes the data in manageable pieces to balance memory usage and performance.
When spectra share identical X data, the algorithm broadcasts the X array only once and shares it
across all spectra in the output workspace, reducing both transmission time and memory footprint.
This is a collective MPI operation, meaning all processes in the communicator must call the algorithm
simultaneously or the job will hang.


Usage
-----

**Example - Simple Broadcast**

The following code should be saved as ``broadcast.py``

.. code-block:: python

    from mantid.simpleapi import CreateWorkspace, BroadcastWorkspace
    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    # Only rank 0 creates the workspace to broadcast
    input_ws = None
    if rank == 0:
        print(f"Rank {rank}: Creating workspace to broadcast")
        x = [0.0, 1.0, 2.0, 3.0, 4.0]
        y = [1.0, 2.0, 3.0, 4.0,
            5.0, 6.0, 7.0, 8.0,
            9.0, 10.0, 11.0, 12.0]
        e = [0.1, 0.1, 0.1, 0.1,
            0.2, 0.2, 0.2, 0.2,
            0.3, 0.3, 0.3, 0.3]

        input_ws = CreateWorkspace(
            DataX=x,
            DataY=y,
            DataE=e,
            NSpec=3,
            OutputWorkspace="input_ws"
        )

        print(f"Rank {rank}: Created workspace with {input_ws.getNumberHistograms()} spectra")

    # All ranks participate in the broadcast
    # Only rank 0 needs to provide InputWorkspace
    if rank == 0:
        output_ws = BroadcastWorkspace(
            InputWorkspace=input_ws,
            BroadcasterRank=0,
            OutputWorkspace="output_ws"
        )
    else:
        # Other ranks don't provide InputWorkspace
        output_ws = BroadcastWorkspace(
            BroadcasterRank=0,
            OutputWorkspace="output_ws"
        )

    # All ranks now have the workspace
    print(f"Rank {rank}: Received workspace with {output_ws.getNumberHistograms()} spectra")
    if rank != 0:
        print(f"Rank {rank}: First spectrum Y values: {output_ws.readY(0)}")
        print(f"Rank {rank}: Last spectrum Y values: {output_ws.readY(2)}")


To run:

.. code-block:: bash

    mpiexec -n 4 broadcast.py

Output:

.. note::

    Due to nature of MPI, the output may be recieved out of order

.. code-block:: text

    Rank 0: Creating workspace to broadcast
    Rank 0: Created workspace with 3 spectra
    Rank 0: Received workspace with 3 spectra
    Rank 1: Received workspace with 3 spectra
    Rank 1: First spectrum Y values: [1. 2. 3. 4.]
    Rank 1: Last spectrum Y values: [ 9. 10. 11. 12.]
    Rank 2: Received workspace with 3 spectra
    Rank 2: First spectrum Y values: [1. 2. 3. 4.]
    Rank 2: Last spectrum Y values: [ 9. 10. 11. 12.]
    Rank 3: Received workspace with 3 spectra
    Rank 3: First spectrum Y values: [1. 2. 3. 4.]
    Rank 3: Last spectrum Y values: [ 9. 10. 11. 12.]


.. categories::

.. sourcelink::
