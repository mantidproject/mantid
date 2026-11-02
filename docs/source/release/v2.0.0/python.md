(v2.0.0-python)=

# Python

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## New API

This release provides a brand new API with many improvements that make scripting Mantid easier and more natural to use.

- For example, the simple algorithm functions now return their output workspace

```{figure} ../../images/2_0_release/NewPythonAPI_Snapshot.webp
---
class: screenshot
width: 600px
align: right
alt: NewPythonAPI Snapshot
---
```

- For a list of changes between the old and new API see [here](http://download.mantidproject.org/docs/releases/2.0/python/html/changes.html). The introduction to {doc}`Python in Mantid </tutorials/python_in_mantid/index>` has also been updated to follow the new API.

- The old API is still available using a simple import

```
 from mantidsimple import *
```

- Please see the {doc}`documentation </tutorials/python_in_mantid/index>` for full descriptions of changes and an how to use the new API.

## Improvements to Plotting

- The control over curves from Python has been improved. Most notably you can now control the line colour and symbol from a script.

- For full descriptions of the functionality see Working with curves

## IPython

As a preview of up coming work we have provided the ability to start an IPython console from within MantidPlot. As this is a preview it will not work out of the box as additional prerequisites are required. The steps required to gain access to the console are described here

Please note that this is a preview and as such is not fully supported yet.

## Notable Bug Fixes

### Crash with Window Handles

A long-standing issue within MantidPlot concerned the lifetime of window handles that had been created in Python, i.e. a graph handle

```
 g = plotSpectrum('ws', 0)
```

The handle was fine until the window was closed. If the handle was used after the window was closed then MantidPlot would crash. This has now been fixed and the object is simply set to `None`
