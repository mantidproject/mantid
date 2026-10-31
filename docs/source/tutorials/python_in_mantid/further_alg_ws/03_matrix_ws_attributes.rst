.. _03_matrix_ws_attributes:

==========================
MatrixWorkspace Attributes
==========================


Spectra and Bins
================

A MatrixWorkspace is a 2D array of data :ref:`divided into spectra and bins <03_workspaces>`.

Get the number of histograms (spectra) in a workspace:

.. code-block:: python

	print(workspace_name.getNumberHistograms())

Get the number of bins:

.. code-block:: python

	print(workspace_name.blocksize())


Read or Extract Data
====================

Access the data using `readX,Y,E` (simply a **view** into an existing workspace) or `extractX,Y,E` (creates a **copy** of the data). These commands take a workspace index and returns a list of the relevant data.

Create a view of the Y data from the 2nd spectrum:

.. code-block:: python

	y_data2 = raw_workspace.y(1)

	for y in y_data2:
	    print(y)


Workspace Objects
=================

There is a lot of information about workspaces that can be accessed.

Below is a brief overview of the most commonly used functionality, for further details see:

* :py:obj:`MatrixWorkspace <mantid.api.MatrixWorkspace>`

getSpectrum for info on the structure of a workspace eg. the spectrum number related to workspace index 0:

.. code-block:: python

    ws.getSpectrum(0).getSpectrumNo()

:ref:`getAxis <mantid.api.MantidAxis>` for Workspace labels and structure eg. Give the AxisUnit a new label:

.. code-block:: python

    ws.getAxis(0).getUnit().setLabel("Time-of-flight", "Milliseconds")

getInstrumentName for the name of the instrument, and :py:obj:`componentInfo <mantid.geometry.ComponentInfo>` for :py:obj:`Sample <mantid.api.Sample>` and Source :ref:`Geometry`.
The ComponentInfo describes every part of the instrument (source, sample, detectors and the banks that hold them) by an index:

.. code-block:: python

    print(ws.getInstrumentName())
    component_info = ws.componentInfo()
    print(component_info.sourcePosition())
    print(component_info.samplePosition())

:ref:`SpectrumInfo` (detector geometry by workspace index), :py:obj:`~mantid.geometry.DetectorInfo` (by detector) and :py:obj:`~mantid.geometry.ComponentInfo` have many other features:

.. code-block:: python

    info = ws.spectrumInfo()
    print(info.hasDetectors(0))


Useful links
============

* :ref:`WorkingWithWorkspaces`
* :py:obj:`MatrixWorkspace <mantid.api.MatrixWorkspace>`
* :ref:`Mantid_api`
* :ref:`concepts contents`
