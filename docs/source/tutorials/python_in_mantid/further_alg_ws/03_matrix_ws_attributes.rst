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

	y_data2 = raw_workspace.readY(1)

	for y in y_data2:
	    print(y)


Workspace Objects
=================

There is a lot of information about workspaces that can be accessed.

Below is a brief overview of the most commonly used functionality, for further details see:

* :ref:`An exhaustive list <mantid.api.MatrixWorkspace>`
* :ref:`A wider explanation <MatrixWorkspace>`

getSpectrum for info on the structure of a workspace eg. the spectrum number related to workspace index 0:

.. code-block:: python

    ws.getSpectrum(0).getSpectrumNo()

:ref:`getAxis <mantid.api.MantidAxis>` for Workspace labels and structure eg. Give the AxisUnit a new label:

.. code-block:: python

    ws.getAxis(0).getUnit().setLabel("Time-of-flight", "Milliseconds")

:ref:`getInstrument <Instrument>` for :ref:`Sample` and Source :ref:`Geometry`.

.. code-block:: python

    instrument = ws.getInstrument()
    print(instrument.getName())

:ref:`SpectrumInfo`, :ref:`DetectorInfo` and :ref:`ComponentInfo` have many other features:

.. code-block:: python

    info = ws.spectrumInfo()
    print(info.hasDetectors(0))


Useful links
============

* :ref:`WorkingWithWorkspaces`
* :ref:`MatrixWorkspace`
* :ref:`Mantid_api`
* :ref:`concepts contents`
