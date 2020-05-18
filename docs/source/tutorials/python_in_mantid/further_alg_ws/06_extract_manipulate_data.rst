.. _06_extract_manipulate_data:

===========================
Extract and Manipulate Data
===========================


Workspace data can be extracted as numpy arrays. Data can be extracted spectrum by spectrum as follows:

.. code-block:: python

	ws = Load(Filename="HRP39182.RAW")
	for i in range(ws.getNumberHistograms()):
	    y = ws.readY(i)
	    x = ws.readX(i)
	    e = ws.readE(i)

This is useful for processing the workspace spectra by spectra. The arrays returned are immutable.

Nested Looping
==============

One way to run computations on data within workspaces is to use a nested loop to access the individual bins on each spectra. In the following we sum the y-values in each spectra.

.. code-block:: python

	ws = Load(Filename="HRP39182.RAW")
	ws = Rebin(InputWorkspace=ws, Params=1e4) # Rebin to make the looping more manageable.
	# Outer loop. Loop over spectrum
	for i in range(ws.getNumberHistograms()):
	    y = ws.readY(i)
	    sum_counts = 0
	    # Inner loop. Loop over bins.
	    for j in range(ws.blocksize()):
	        sum_counts += y[j] 
	    # Display spectrum number against sum_counts
	    print("{0} {1}".format(ws.getSpectrum(i).getSpectrumNo(), sum_counts))

Creating Output Workspaces
==========================

The following creates a new workspace from the first spectra of the loaded workspace

.. code-block:: python

	y = ws.readY(0)
	x = ws.readX(0)
	e = ws.readE(0)
	out_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1)

We may perform some processing on the read data arrays before creating our new workspace. The following changes to x-axis for TOF in microseconds to TOF in miliseconds.

.. code-block:: python

	ws = Load(Filename="HRP39182.RAW")	
	x = ws.readX(0)
	y = ws.readY(0)
	e = ws.readE(0)
	new_x = x * 1e-3
	new_ws = CreateWorkspace(DataX=new_x, DataY=y, DataE=e, NSpec=1,UnitX='Label')
	unit = new_ws.getAxis(0).getUnit()
	unit.setLabel("Time-of-flight", "Milliseconds")

The data from all spectra can be obtained as a mutable multi-dimensional array in one-call using the extract methods.

.. code-block:: python

	ws = Load(Filename="HRP39182.RAW")
	x = ws.extractX()
	y = ws.extractY()
	e = ws.extractE()
	print(x.shape)
	print(y.shape)
	print(e.shape)