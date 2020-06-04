.. _06_extract_manipulate_data:

===========================
Extract and Manipulate Data
===========================

Read or Extract Data
====================

Read produces a view into the chosen part of the original data.
Extract creates a copy of this part of the data.

Read out the Y data for the second spectrum:

.. code-block:: python

		y_data2 = raw_workspace.readY(1)
	for y in y_data2:
	    print(y)

Using a for loop, read and print the first value in all spectra

.. code-block:: python

	for index in range(0, raw_workspace.getNumberHistograms()):
	    #Note the round brackets followed by the square brackets
	    print(raw_workspace.readY(index)[0])

Workspace data can be read as numpy arrays, spectrum by spectrum:

.. code-block:: python

	ws = Load(Filename="HRP39182.RAW")
	for i in range(ws.getNumberHistograms()):
	    y = ws.readY(i)
	    x = ws.readX(i)
	    e = ws.readE(i)

**Be careful**: the outputs of *read* (y,x,e) are only **views into the data held by the workspace,** `ws`. If `ws` is deleted, the contents of x,y,e will be nonsense (the random contents of the memory locations formerly used for `ws`).
If you need x,y,e to persist longer than ws, use the *extract*, which creates a copy of the data in `ws` into y,x,e.

Nested Looping
==============

This allows access the individual bins in each spectrum. In the following we sum the y-values in each spectrum:

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
	    print("Spectrum Number: {0}, Total Counts: {1}".format(ws.getSpectrum(i).getSpectrumNo(), sum_counts))

Creating Output Workspaces
==========================

Create a new workspace from the first spectra of the loaded workspace

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

Since the extract methods return multi-dimensional numpy arrays, if you want to use them to replace the read methods mentioned above, instead of `ws.readX(5)` you should use `ws.extractX()[5, :]` or `xmat = ws.extractX(); x = xmat[5, :]`.
