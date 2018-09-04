.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the incident energy from the time-of-flight between one monitor and some detectors. The time information is extracted from the 'PeakCentre' columns of the EPP workspaces. The 'FitSuccess' column in the EPP tables is used to single out detectors without good quality elastic peaks: only detectors with ``success`` in the column are accepted. Monitor-to-sample and sample-to-detector distances are loaded from the instrument definition. Both the time and the distance data is averaged over the specified detectors. 

The EPP tables can be produced using the :ref:`algm-FindEPP` algorithm.

If no *MonitorWorkspace* is specified, the monitor spectrum is expected to be in the detector workspace.

Specifying the detectors and monitor
####################################

The drop-down menu is used to specify what the numbers in the *Detectors* and *Monitor* fields mean. The *Detectors* field understands complex expression, for example ``2,3,5-7,101`` would use detectors 2, 3, 5, 6, 7, and 101 for the computation.

Neutrons detected in later frames
#################################

It is possible that a neutron pulse is detected at the detectors in a later frame than at the monitor. These cases can be identified if the time-of-flight from the monitor to the detectors is negative or if the calculated incident energy would end up being too large. In these cases, the value of  the *PulseInterval* field (in micro seconds) is added to the time-of-flight until the result is satisfactory. If *PulseInterval* is not given, the algorithm uses the 'pulse_interval' sample log (in seconds) instead.

To identify when the incident energy is within acceptable bounds, the algorithm applies simple heuristics. Basically, the final energy has to be within 20% of the *NominalIncidentEnergy* or within the *PulseInterval* corrected time-of-flight :math:`\pm` *PulseInterval* / 2.

Usage
-----

**Example - simple incident energy calibration:**

.. testcode:: ExGetEiMonDet

	import numpy
	from scipy.constants import elementary_charge, neutron_mass

	# We need some definitions for a fake instrument and data.
	# Nominal (ideal) incident energy, in meV.
	E_i = 55.0
	# In reality, the energy is something else.
	realE_i = 0.98 * E_i
	# Position of peaks in microseconds.
	# Has to be the same for monitor and detectors because of
	# the way the fake instrument is created. This also implies
	# that the neutron pulse will arrive at the detectors in the
	# next frame.
	peakPosition = 300.0
	pulseInterval = 1500.0
	# Specify/calculate instrument dimensions.
	monitorToSample = 0.6
	# Remember, the neutrons are detected in the next frame.
	timeOfFlight = pulseInterval
	velocity = numpy.sqrt(2 * realE_i * elementary_charge * 1e-3 / neutron_mass)
	sampleToDetector = velocity * timeOfFlight * 1e-6 - monitorToSample

	# Fake workspace creation. This is a bit ugly hack, but we
	# will create a workspace with two detector banks and eventually
	# move one of them where the monitor is supposed to be pretending
	# that it actually is the monitor.

	# First the spectra. They contain a single peak at peakPosition.
	spectrum = 'name = Gaussian, PeakCentre = {0}, Height = 500.0, Sigma = 30.0'.format(peakPosition)
	# The workspace itself.
	ws = CreateSampleWorkspace(WorkspaceType = 'Histogram', XUnit = 'TOF',
	XMin = 100.0, XMax = 1100.0, BinWidth = 10.0,
	NumBanks = 2, BankDistanceFromSample = sampleToDetector,
	Function = 'User Defined', UserDefinedFunction = spectrum)
	# Move the detector.
	MoveInstrumentComponent(Workspace = ws, ComponentName = 'basic_rect/bank2',
	X = -monitorToSample,
	RelativePosition = False)

	# Preparations are done, actual calibration ensues.
	eppTable = FindEPP(InputWorkspace = ws)
	# We choose all detectors in the detector bank, and only the
	# centre detector as the monitor in the monitor bank.
	calibratedE_i = GetEiMonDet(DetectorWorkspace = ws, DetectorEPPTable = eppTable,
	Detectors = "100-199", Monitor = 200,
	NominalIncidentEnergy = E_i, PulseInterval = pulseInterval, Version=2)

	print('Nominal incident energy: {0:.5f}'.format(E_i))
	print('Calibrated energy: {0:.5f}'.format(calibratedE_i))
	print('Real energy: {0:.5f}'.format(realE_i))

Output:

.. testoutput:: ExGetEiMonDet

	Nominal incident energy: 55.00000
	Calibrated energy: 53.90968
	Real energy: 53.90000

Previous versions
-----------------

Version 1
#########

The first version of this algorithm was written as a temporary replacement for :ref:`algm-GetEi`. In version 2, the algorithm was completely overhauled. The main differences are:
	- Instead of doing its own peak finding (via :ref:`algm-GetEi`), version 2 utilizes the EPP tables.
	- The detectors can be specified now by indices instead of distances.
	- The pulse interval can be taken into account.

.. categories::

.. sourcelink::
