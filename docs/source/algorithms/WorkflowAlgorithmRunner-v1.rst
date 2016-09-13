# Managing data reductions with `WorkspaceAlgorithmRunner`

When reducing the raw data of a complete neutron experiment, it is typical that dependencies occur between the datasets. For example, for vanadium normalisation, the vanadium run has to be processed before the samples and if an empty vanadium run exists, it has to be processed before the vanadium. Trying to incorporate such dependency resolution to the workflow algorithm tends to make the algorithm unnecessarily complex. One solution is to keep the workflow algorithm as simple as possible and outsource the dependency resolution to a 'master' or 'manager' algorithm. This algorithm is called `WorkspaceAlgorithmRunner`. For `WorkspaceAlgorithmRunner`, one specified all the datasets to be processed and how they depend on each other. The algorithm then declares the order of processing and controls the information flow between the reductions. 

## Managed algorithm requirements



## Setting up the reductions: `SetupTable`

`SetupTable` is a Mantid table workspace where each row corresponds to a single run of the managed algorithm. The first column must be named 'Id' and contain a unique identifier string for each row. The rest of the columns correspond to the input and output properties of the managed algorithm and must have the same names and corresponding types as in the managed algorithm. Note, that workspaces are always referenced by their name string. Of these columns, 'WorkspaceAlgorithmRunner' processes only the ones dealing with input and output workspaces and exlicitely specified in `InputOutputMap`. The rest of the values in the columns are passed to the managed algorithm as-is.

## Connecting inputs and outputs: `InputOutputMap`



## Hard coded inputs

## Forced output

