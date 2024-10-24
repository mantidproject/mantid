.. _08_running_algorithms_complex:

====================================
Running Algorithms - Complex Example
====================================


We have seen an example of chaining together two algorithms where just the workspace was passed as input. While this is a powerful use of Mantid it is also possible to extract the workspace data out to numpy, perform some operations and then feed this back into a workspace.

The script below shows a more complex example of mixing Mantid's algorithms with numpy

.. code-block:: python

	import numpy

	white_beam = Load("MAR11060.raw")

	# Define constants
	norm_spectrum = 2
	wb_scale_factor = 100.
	rb_params = [10.,1., 80.]

	white_beam = NormaliseToMonitor(white_beam,MonitorSpectrum=norm_spectrum)
	# crop out monitors
	white_beam = CropWorkspace(white_beam, StartWorkspaceIndex=4)
	white_beam = ConvertUnits(white_beam, Target='Energy')
	white_beam = Rebin(white_beam, rb_params)

	""" Extract data to numpy, computes the sum value for each spectra
	      and puts the results back into a worksapace.
	"""
	xvalues = white_beam.extractX() # 2D copy of workspace data
	yvalues = white_beam.extractY() # 2D copy of workspace data
	evalues = white_beam.extractE() # 2D copy of workspace data

	# sum across axis that numpy calls 1, i.e sum values across bins for each spectra
	sumy = numpy.sum(yvalues, axis=1)
	#Compute errors
	evalues = evalues ** 2 # Square each value
	evalues = numpy.sum(evalues, axis=1) # Sum the squares
	evalues = numpy.sqrt(evalues)

	xlimits = xvalues[ :, [0,-1] ] # numpy array slice. : takes all rows and [0,-1] selects only column 0 and n-1
	# + any other operations that numpy can do

	# Put back into workspace.
	white_beam = CreateWorkspace(xlimits, sumy, evalues,NSpec=sumy.shape[0])

	# Divide sample run
	sample = Load("MAR11015")
	sample = NormaliseToMonitor(sample,MonitorSpectrum=norm_spectrum)
	sample = ConvertUnits(sample, Target='DeltaE',EMode='Direct',EFixed=85)
	# crop out monitors
	sample = CropWorkspace(sample, StartWorkspaceIndex=4)
	sample = Rebin(sample, rb_params)

	# Normalize
	sample /= white_beam
