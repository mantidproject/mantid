.. _06_alg_help:

==============
Algorithm Help
==============

Each of the algorithm functions has help documentation, which can be accessed using the Python `help()` command.

To show the purpose of a particular algorithm and examine the expected inputs, we may use the help command with the algorithm name as an argument.

For example

.. code-block:: python

    help(Rebin)

produces

.. code-block:: python

    Help on function Rebin in module mantid.simpleapi:

    Rebin(InputWorkspace,Params,PreserveEvents, Version=1)
        Rebins data with new X bin boundaries. For EventWorkspaces, you can very quickly rebin in-place by keeping the same output name and PreserveEvents=true.

        Property descriptions:

        InputWorkspace(Input:req) *MatrixWorkspace*       Workspace containing the input data

        OutputWorkspace(Output:req) *MatrixWorkspace*       The name to give the output workspace

        Params(Input:req) *dbl list*       A comma separated list of first bin boundary, width, last bin boundary. Optionally
        this can be followed by a comma and more widths and last boundary pairs.
        Negative width values indicate logarithmic binning.

        PreserveEvents(Input) *boolean*       Keep the output workspace as an EventWorkspace, if the input has events (default).
        If the input and output EventWorkspace names are the same, only the X bins are set, which is very quick.
        If false, then the workspace gets converted to a Workspace2D histogram.
