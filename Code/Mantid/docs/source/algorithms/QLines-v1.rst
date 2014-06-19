.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The model that is being fitted is that of a δ-function (elastic
component) of amplitude :math:`A(0)` and Lorentzians of amplitude
:math:`A(j)` and HWHM :math:`W(j)` where :math:`j=1,2,3`. The whole
function is then convolved with the resolution function. The -function
and Lorentzians are intrinsically normalised to unity so that the
amplitudes represent their integrated areas.

For a Lorentzian, the Fourier transform does the conversion:
:math:`1/(x^{2}+\delta^{2}) \Leftrightarrow exp[-2\pi(\delta k)]`. If
:math:`x` is identified with energy :math:`E` and :math:`2\pi k` with
:math:`t/\hbar` where t is time then:
:math:`1/[E^{2}+(\hbar / \tau )^{2}] \Leftrightarrow exp[-t /\tau]` and
:math:`\sigma` is identified with :math:`\hbar / \tau`. The program
estimates the quasielastic components of each of the groups of spectra
and requires the resolution file and optionally the normalisation file
created by ResNorm.

For a Stretched Exponential, the choice of several Lorentzians is
replaced with a single function with the shape :
:math:`\psi\beta(x) \Leftrightarrow exp[-2\pi(\sigma k)\beta]`. This, in
the energy to time FT transformation, is
:math:`\psi\beta(E) \Leftrightarrow exp[-(t/\tau)\beta]`. So \\sigma is
identified with :math:`(2\pi)\beta\hbar/\tau`. The model that is fitted
is that of an elastic component and the stretched exponential and the
program gives the best estimate for the :math:`\beta` parameter and the
width for each group of spectra.

This routine was originally part of the MODES package.

Usage
-----
**Example - a basic example using QLines to fit a reduced workspace.**

.. code-block:: python

		def createSampleWorkspace(name, random=False):
			""" Creates a sample workspace with a single lorentzian that looks like IRIS data"""
			import os
			function = "name=Lorentzian,Amplitude=8,PeakCentre=5,FWHM=0.7"
			ws = CreateSampleWorkspace("Histogram", Function="User Defined", UserDefinedFunction=function, XUnit="DeltaE", Random=True, XMin=0, XMax=10, BinWidth=0.01)
			ws = CropWorkspace(ws, StartWorkspaceIndex=0, EndWorkspaceIndex=9)
			ws = ScaleX(ws, -5, "Add")
			ws = ScaleX(ws, 0.1, "Multiply")
			
			#load instrument and instrument parameters
			LoadInstrument(ws, InstrumentName='IRIS')
			path = os.path.join(config['instrumentDefinition.directory'], 'IRIS_graphite_002_Parameters.xml')
			LoadParameterFile(ws, Filename=path)
			ws = RenameWorkspace(ws, OutputWorkspace=name)
			return ws


		ws = createSampleWorkspace("irs26176_graphite002_red", random=True)
		res = createSampleWorkspace("irs26173_graphite002_res")

		QLines(SamNumber='26176', ResNumber='26173', InputType='Workspace', ResInputType='Workspace', Instrument='irs', Analyser='graphite002', Plot='None')

.. categories::
