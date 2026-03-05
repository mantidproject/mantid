.. _MPI:

MPI in Mantid
=============

What is MPI?
------------

MPI (Message Passing Interface) is a standardized and portable message-passing system
designed to enable parallel computing across multiple processors or nodes.
It allows programs to coordinate and communicate data between processes running simultaneously,
making it possible to distribute computational workloads and process large datasets more efficiently.

In the context of scientific computing and data reduction, MPI enables:

- **Parallel data processing** - Split large datasets across multiple processes for faster analysis
- **Distributed memory computing** - Each process has its own memory space, allowing processing of datasets larger than a single machine's memory
- **Scalability** - Run the same code on anything from a laptop to a supercomputer cluster
- **Collective operations** - Built-in operations for common patterns like broadcasting, gathering, and reducing data

MPI in Mantid
-------------

Mantid provides MPI support through a few specialized algorithms that enable parallel data reduction workflows.
These algorithms are designed to work with Mantid's workspace objects as MPI's communication primitives.

The MPI algorithms in Mantid are:

- **BroadcastWorkspace** - Distributes a workspace from one process to all other processes
- **GatherWorkspaces** - Collects workspaces from all processes to a single root process

These algorithms are built as part of the ``MantidMPIAlgorithms`` plugin and are only available for linux in the ``mantidmpi`` package
or when Mantid is compiled with the cmake flag ``MPI_BUILD=ON``.

Building Mantid with MPI Support
---------------------------------

To use MPI algorithms, Mantid must be compiled with MPI support enabled:

.. code-block:: bash

    cmake -DMPI_BUILD=ON --preset=linux
    cd build
    ninja

The required MPI packages are already included as mantid dependencies.

Running Mantid Scripts with MPI
--------------------------------

MPI-enabled Mantid scripts are executed using the ``mpiexec`` launcher:

.. code-block:: bash

    mpiexec -np 4 python my_mpi_script.py

Where:

- ``-np 4`` specifies the number of MPI processes (ranks) to launch
- Each process executes the same script but can take different code paths based on its rank

Common MPI Patterns
-------------------

Master-Worker Pattern
^^^^^^^^^^^^^^^^^^^^^

The most common pattern involves a "master" process (typically rank 0) that coordinates work:

.. code-block:: python

    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    if rank == 0:
        # Master process
        print(f"Running with {size} processes")
        # Coordinate work
    else:
        # Worker processes
        # Perform assigned tasks
        pass

Broadcast-Process-Gather Pattern
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A typical parallel data reduction workflow:

1. **Broadcast** - Distribute common data (workspace, calibration, instrument geometry) to all processes
2. **Process** - Each process works on its portion of the data independently
3. **Gather** - Collect results back to the master process

MPI Algorithm Reference
-----------------------

BroadcastWorkspace
^^^^^^^^^^^^^^^^^^

**Purpose**: Distribute a workspace from one MPI process (the broadcaster) to all other processes in the MPI job.

**Key Features**:

- Only the broadcaster rank needs to provide an input workspace
- Optimized for large workspaces through chunked broadcasting
- Automatically detects and optimizes shared X data across spectra
- Memory-efficient with configurable chunk sizes

**Usage**: This is a collective MPI operation - all processes must call it simultaneously.

GatherWorkspaces
^^^^^^^^^^^^^^^^

**Purpose**: Collect workspaces from all MPI processes to a single root process.

**Key Features**:

- Supports two accumulation methods: Add and Append
- Chunked processing for large workspaces
- Preserves EventWorkspace data when requested
- Only root process receives output

**Usage**: This is a collective MPI operation - all participating processes must call it simultaneously.

Examples
--------

Example 1: Broadcasting Calibration Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A common use case is to load a workspace or calibration data on one process and distribute it to all others:

.. code-block:: python

    from mantid.simpleapi import Load, BroadcastWorkspace
    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()

    # Only rank 0 loads the calibration file
    if rank == 0:
        cal_ws = Load(Filename='calibration.nxs')
        shared_cal = BroadcastWorkspace(
            InputWorkspace=cal_ws,
            BroadcasterRank=0,
            OutputWorkspace='calibration'
        )
    else:
        # Other ranks receive the broadcast
        shared_cal = BroadcastWorkspace(
            BroadcasterRank=0,
            OutputWorkspace='calibration'
        )

    # All ranks now have 'shared_cal' and can use it for processing

Example 2: Parallel Data Reduction with Gather
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Process different run numbers in parallel and combine results:

.. code-block:: python

    from mantid.simpleapi import Load, Rebin, GatherWorkspaces
    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    # Define run numbers to process
    run_numbers = [12345, 12346, 12347, 12348, 12349, 12350, 12351, 12352]

    # Each rank processes a subset of runs
    my_runs = run_numbers[rank::size]  # Distribute runs round-robin

    # Process assigned runs
    for run_num in my_runs:
        ws = Load(Filename=f'data_{run_num}.nxs')
        ws = Rebin(InputWorkspace=ws, Params='0.1,-0.001,3.0')
        # Additional processing...

    # For this example, assume last processed workspace is 'ws'
    # Gather all processed workspaces to rank 0
    if rank == 0:
        combined = GatherWorkspaces(
            InputWorkspace=ws,
            AccumulationMethod='Append',
            OutputWorkspace='all_runs_combined'
        )
        print(f"Combined workspace has {combined.getNumberHistograms()} spectra")
    else:
        GatherWorkspaces(
            InputWorkspace=ws,
            AccumulationMethod='Append'
        )

Example 3: Summing Data Across Processes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use "Add" mode to sum corresponding spectra from all processes:

.. code-block:: python

    from mantid.simpleapi import CreateWorkspace, GatherWorkspaces
    from mpi4py import MPI
    import numpy as np

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()

    # Each rank creates workspace with rank-dependent values
    x = np.linspace(0, 10, 101)
    y = np.ones(100) * (rank + 1)  # Rank 0: 1s, Rank 1: 2s, etc.

    ws = CreateWorkspace(DataX=x, DataY=y, NSpec=1)

    # Sum all workspaces together
    if rank == 0:
        summed = GatherWorkspaces(
            InputWorkspace=ws,
            AccumulationMethod='Add',
            OutputWorkspace='summed'
        )
        # If 4 processes: result = 1 + 2 + 3 + 4 = 10
        print(f"Summed value: {summed.readY(0)[0]}")
    else:
        GatherWorkspaces(
            InputWorkspace=ws,
            AccumulationMethod='Add'
        )

Example 4: Complete Workflow - Broadcast, Process, Gather
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A complete parallel data reduction workflow:

.. code-block:: python

    from mantid.simpleapi import (Load, BroadcastWorkspace,
                                   AlignAndFocusPowder, GatherWorkspaces)
    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    # Step 1: Broadcast calibration data
    if rank == 0:
        cal = Load(Filename='calibration.nxs')
        cal = BroadcastWorkspace(InputWorkspace=cal,
                                 BroadcasterRank=0,
                                 OutputWorkspace='cal')
    else:
        cal = BroadcastWorkspace(BroadcasterRank=0,
                                OutputWorkspace='cal')

    # Step 2: Each rank processes its assigned data
    my_file = f'data_rank{rank}.nxs'
    ws = Load(Filename=my_file)

    # Use the shared calibration
    ws = AlignAndFocusPowder(InputWorkspace=ws,
                            CalFileName=cal,
                            Params='0.5,-0.001,10')

    # Step 3: Gather results
    if rank == 0:
        combined = GatherWorkspaces(InputWorkspace=ws,
                                   AccumulationMethod='Append',
                                   OutputWorkspace='final_result')
        # Save combined result
        SaveNexus(InputWorkspace=combined,
                 Filename='combined_output.nxs')
    else:
        GatherWorkspaces(InputWorkspace=ws,
                        AccumulationMethod='Append')

Performance Considerations
--------------------------

Chunking Strategy
^^^^^^^^^^^^^^^^^

Both ``BroadcastWorkspace`` and ``GatherWorkspaces`` support chunked processing:

- **Automatic (ChunkSize=0)** - Recommended for most cases, targets ~100MB chunks
- **All-at-once (ChunkSize=-1)** - Fastest but uses most memory
- **Explicit (ChunkSize=N)** - Manual control, useful for memory-constrained systems

Memory Usage
^^^^^^^^^^^^

- ``BroadcastWorkspace`` temporarily duplicates data during transmission
- ``GatherWorkspaces`` with Append mode requires memory for all gathered data on root
- Consider using chunking and processing data in stages for very large datasets

Network Bandwidth
^^^^^^^^^^^^^^^^^

- Shared X data is automatically detected and broadcast only once
- Use appropriate chunk sizes to balance memory and network efficiency
- EventWorkspaces can be large - consider converting to histograms if events aren't needed

Common Pitfalls
---------------

Collective Operations
^^^^^^^^^^^^^^^^^^^^^

**Problem**: One process doesn't call the MPI algorithm, causing the job to hang.

**Solution**: Ensure all processes in the communicator call MPI algorithms, even if they don't have input data.

.. code-block:: python

    # WRONG - only rank 0 calls the algorithm
    if rank == 0:
        output = GatherWorkspaces(InputWorkspace=ws)

    # CORRECT - all ranks participate
    if rank == 0:
        output = GatherWorkspaces(InputWorkspace=ws,
                                 AccumulationMethod='Append',
                                 OutputWorkspace='result')
    else:
        GatherWorkspaces(InputWorkspace=ws,
                        AccumulationMethod='Append')

Workspace Compatibility
^^^^^^^^^^^^^^^^^^^^^^^

**Problem**: Attempting to gather workspaces with different numbers of bins.

**Solution**: Ensure all workspaces have compatible structure before gathering:

.. code-block:: python

    # All workspaces must have same number of bins
    # Rebin to common grid if necessary
    ws = Rebin(InputWorkspace=ws, Params='0,-0.001,10')

Missing Input on Root
^^^^^^^^^^^^^^^^^^^^^

**Problem**: Root process doesn't provide input to ``BroadcastWorkspace``.

**Solution**: Only the broadcaster rank needs to provide input:

.. code-block:: python

    if rank == 0:
        ws = Load(Filename='data.nxs')
        output = BroadcastWorkspace(InputWorkspace=ws)  # Required
    else:
        output = BroadcastWorkspace(BroadcasterRank=0)  # No input needed

Additional Resources
--------------------

- MPI Tutorial: https://mpitutorial.com/
- mpi4py Documentation: https://mpi4py.readthedocs.io/
- Mantid MPI Algorithms: See :ref:`BroadcastWorkspace <algm-BroadcastWorkspace>` and :ref:`GatherWorkspaces <algm-GatherWorkspaces>`

.. categories:: Concepts
