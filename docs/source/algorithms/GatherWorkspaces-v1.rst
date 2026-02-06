.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

GatherWorkspaces collects workspaces from all MPI processes and combines them on the root process (rank 0).
The algorithm supports two accumulation methods:
"Add" mode sums corresponding spectra across all processes (adding Y values and combining errors in quadrature),
while "Append" mode concatenates all spectra to create a larger workspace.
Only processes with input workspaces participate in the gather operation, and only the root process produces an output workspace.
The algorithm uses chunked processing to handle large workspaces efficiently, balancing memory usage with communication overhead.
All input workspaces must have the same number of bins and must all be either histogram or point data.
For EventWorkspaces, the algorithm can preserve event-level data if requested,
otherwise it converts to histogram format to reduce memory usage.
This is a collective MPI operation requiring all participating processes to call the algorithm simultaneously.


Usage
-----

**Example - Append**

The following code should be saved as ``gather.py``

.. code-block:: python

    from mantid.simpleapi import CreateWorkspace, GatherWorkspaces
    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()

    # Each rank creates a rank-local workspace
    x = [0.0, 1.0, 2.0, 3.0]
    y = [rank + 1.0, rank + 2.0, rank + 3.0]
    e = [1.0, 1.0, 1.0]

    input_ws = CreateWorkspace(
        DataX=x,
        DataY=y,
        DataE=e,
        NSpec=1,
        OutputWorkspace=f"input_rank_{rank}"
    )

    output_ws = GatherWorkspaces(
        InputWorkspace=input_ws,
        AccumulationMethod="Append"
    )

    if rank == 0:
        print("Number of spectra in output:", output_ws.getNumberHistograms())
        for i in range(output_ws.getNumberHistograms()):
            print(f"Spectrum {i} Y:", output_ws.readY(i))

To run:

.. code-block:: bash

    mpiexec -n 4 gather.py

Output:

.. code-block:: text

    Number of spectra in output: 4
    Spectrum 0 Y: [1. 2. 3.]
    Spectrum 1 Y: [2. 3. 4.]
    Spectrum 2 Y: [3. 4. 5.]
    Spectrum 3 Y: [4. 5. 6.]


.. categories::

.. sourcelink::
