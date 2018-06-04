.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Create a PropertyManager object setting the reduction options for ILL
SANS instruments. The property manager object is then added to
the PropertyManagerDataService.
This was based on the :ref:`SetupEQSANSReduction <algm-SetupEQSANSReduction>`.

See :ref:`SANSReduction <algm-SANSReduction>` for details.

Usage
-----

**Example - Reduces D33 data from a set of NeXus files:**

.. code-block:: python

	# List of detector IDs masked
	maskedDetectors = [14709,14710,14711,14712,14713,14714,14715,14716,14717,14718,
		14719,14720,14721,14722,14723,14724,14725,14726,14727,14728,14729,14730,14731,
		14732,14733,14734,14735,14965,14966,14967,14968,14969,14970,14971,14972,14973,
		14974,14975,14976,14977,14978,14979,14980,14981,14982,14983,14984,14985,14986,
		14987,14988,14989,14990,14991,15221,15222,15223,15224,15225,15226,15227,15228,
		15229,15230,15231,15232,15233,15234,15235,15236,15237,15238,15239,15240,15241,
		15242,15243,15244,15245,15246,15247,15477,15478,15479,15480,15481,15482,15483,
		15484,15485,15486,15487,15488,15489,15490,15491,15492,15493,15494,15495,15496,
		15497,15498,15499,15500,15501,15502,15503,15733,15734,15735,15736,15737,15738,
		15739,15740,15741,15742,15743,15744,15745,15746,15747,15748,15749,15750,15751,
		15752,15753,15754,15755,15756,15757,15758,15759,15989,15990,15991,15992,15993,
		15994,15995,15996,15997,15998,15999,16000,16001,16002,16003,16004,16005,16006,
		16007,16008,16009,16010,16011,16012,16013,16014,16015,16245,16246,16247,16248,
		16249,16250,16251,16252,16253,16254,16255,16256,16257,16258,16259,16260,16261,
		16262,16263,16264,16265,16266,16267,16268,16269,16270,16271,16501,16502,16503,
		16504,16505,16506,16507,16508,16509,16510,16511,16512,16513,16514,16515,16516,
		16517,16518,16519,16520,16521,16522,16523,16524,16525,16526,16527,16757,16758,
		16759,16760,16761,16762,16763,16764,16765,16766,16767,16768,16769,16770,16771,
		16772,16773,16774,16775,16776,16777,16778,16779,16780,16781,16782,16783,17013,
		17014,17015,17016,17017,17018,17019,17020,17021,17022,17023,17024,17025,17026,
		17027,17028,17029,17030,17031,17032,17033,17034,17035,17036,17037,17038,17039,
		17269,17270,17271,17272,17273,17274,17275,17276,17277,17278,17279,17280,17281,
		17282,17283,17284,17285,17286,17287,17288,17289,17290,17291,17292,17293,17294,
		17295,17525,17526,17527,17528,17529,17530,17531,17532,17533,17534,17535,17536,
		17537,17538,17539,17540,17541,17542,17543,17544,17545,17546,17547,17548,17549,
		17550,17551]

	# Set all the necessary parameters to reduce the data
	SetupILLSANSReduction(
		MaskedDetectorList=maskedDetectors,
		BeamCenterMethod="DirectBeam",
		BeamCenterFile="ILLD33_direct_beam_transm_001427.nxs",
		Normalisation="Timer",
		DarkCurrentFile= "ILLD33_b4c_001420.nxs",
		TransmissionMethod="DirectBeam",
		TransmissionSampleDataFile= "ILLD33_sample_transm_001431.nxs",
		TransmissionEmptyDataFile= "ILLD33_direct_beam_transm_001427.nxs",
		BckTransmissionEmptyDataFile= "ILLD33_direct_beam_transm_001427.nxs",
		TransmissionBeamRadius = 3,
		TransmissionUseSampleDC=False,
		BackgroundFiles= "ILLD33_empty_cell_001422.nxs",
		BckTransmissionSampleDataFile="ILLD33_empty_cell_transm_001428.nxs",
		DoAzimuthalAverage=False,
		Do2DReduction=False,
		ComputeResolution=True,
		ReductionProperties="d33")

	# Reduce the sample data
	output=SANSReduction(Filename="ILLD33_sample_001425.nxs", ReductionProperties="d33",OutputWorkspace="d33out")

	# Rebin
	Rebin(InputWorkspace='d33out',OutputWorkspace='d33out_rebin',Params='4,0.1,15')

	# Do a radial integration
	SANSAzimuthalAverage1D(InputWorkspace='d33out_rebin',Binning='0.001,0.0002,0.03',OutputWorkspace='IQ_curve')

	# Get the OutputWorkspace as a python workspace
	iq = mtd['IQ_curve']
	print("This workspace has {} dimensions and has {} histograms.".format(iq.getNumDims(), iq.getNumberHistograms()))

Output:

	This workspace has 2 dimensions and has 1 histograms.

.. categories::

.. sourcelink::
