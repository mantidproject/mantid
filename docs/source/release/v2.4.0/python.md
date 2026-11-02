(v2.4.0-python)=

# Python

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## New Features

### Set whole numpy array to workspace spectrum

- It is now possible to set a whole numpy array to a workspace spectrum in a single shot from a Python algorithm. The previous code relied on repeated calls to .data{X,Y,E} methods for each bin. For example

```
 nspectra = 2
 xlength = 11
 ylength = 10

 test_ws = WorkspaceFactory.create("Workspace2D", nvectors, xlength, ylength)
 ws_index = 1

 y_values = np.linspace(0,1,xlength)
 test_ws.setX(ws_index, y_values)

 x_values = np.ones(ylength)
 test_ws.setY(ws_index, x_values)

 e_values = np.sqrt(values)
 test_ws.setE(ws_index, e_values)
```

will set all of the values in a single shot. The lengths of the arrays must match or a `RuntimeError` is raised.

## Bugs Fixed

- Calling an algorithm with the same input & output workspace within a PythonAlgorithm failed. This has been fixed to correctly set the child status of the internal algorithm.
- Python binary operations with GroupWorkspaces left the temporary workspaces around. These are now cleaned up at the end.
- Rerunning a script if a workspace was renamed in the GUI produces an unexpected workspace name. The following steps demonstrate this bug:
  1. Run this is a Python window to produce a workspace called 'ws1': ws1 = Load('MAR11001.raw')
  1. Use the dock to rename the workspace to 'ws2'
  1. Rerun the above python snippet.
  1. Instead of producing a new ws1, it will produce ws2
  - This bug has been fixed. As a consequence some algorithm calls inside of functions will behave slightly differently. In order for an algorithm inside a function to still use the name of output, it must be specified by hand, i.e.

```
 def convert(workspace):
    # Replace the input workspace
    converted = simpleapi.ConvertUnits(InputWorkspace=workspace, OutputWorkspace=workspace,Target='Energy')
    return converted

raw = simpleapi.LoadRaw('IRS21360.raw',SpectrumMax=1)
raw = convert(raw)
```
