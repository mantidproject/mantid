.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm allows the loading, reduction, and analysis of forward scattering data obtained using Deep Inelastic Neutron Scattering (DINS), also referred to as Neutron Compton Scattering (NCS), at the VESUVIO spectrometer.
The algorithm has been developed by the VESUVIO Instrument Scientists, Giovanni Romanelli and Matthew Krzystyniak.
A previous version of the algorithm was described in: G. Romanelli et al.; Journal of Physics: Conf. Series 1055 (2018) 012016.

DINS allows the direct measurements of nuclear kinetic energies and momentum distributions, thus accessing the importance of nuclear quantum effects in condensed-matter systems, as well as the degree of anisotropy and anharmonicity in the local potentials affecting nuclei.
DINS data appear as a collection of mass-resolved peaks (Neutron Compton Profiles, NCPs), that are fitted independently in the time-of-flight spectra, using the formalism of the Impulse Approximation and the y-scaling introduced by G. B. West.

Additional information about DINS theory and applications can be found in the recent review:
C. Andreani et al., Advances in Physics, 66 (2017) 1-73

Additional information on the geometry and operations of the VESUVIO spectrometer can be found in
J. Mayers and G. Reiter, Measurement Science and Technology, 23 (2012) 045902
G. Romanelli et al., Measurement Science and Technology, 28 (2017), 095501


Warning
#######

This algorithm is still in development.
If you encounter any problems please contact the Mantid team and the Vesuvio scientists.

Usage:
------

**Example: Analysis of polyethylene**

.. testsetup:: ExVesuvioAnalysis

    default_facility_orig = config['default.facility']
    default_instrument_orig = config['default.instrument']
    config['default.facility'] = 'ISIS'
    config['default.instrument'] = 'Vesuvio'

.. testcode:: ExVesuvioAnalysis

   # create table of elements
   table = CreateEmptyTableWorkspace()
   table.addColumn(type="str", name="Symbol")
   table.addColumn(type="double", name="Mass (a.u.)")
   table.addColumn(type="double", name="Intensity lower limit")
   table.addColumn(type="double", name="Intensity value")
   table.addColumn(type="double", name="Intensity upper limit")
   table.addColumn(type="double", name="Width lower limit")
   table.addColumn(type="double", name="Width value")
   table.addColumn(type="double", name="Width upper limit")
   table.addColumn(type="double", name="Centre lower limit")
   table.addColumn(type="double", name="Centre value")
   table.addColumn(type="double", name="Centre upper limit")
   table.addRow(['H', 1.0079,  0.,1.,9.9e9,  3.,  4.5,  6.,  -1.5, 0., 0.5])
   table.addRow(['C', 12.0,    0.,1.,9.9e9,  10., 15.5, 30., -1.5, 0., 0.5])

   # create table of constraints
   constraints = CreateEmptyTableWorkspace()
   constraints.addColumn(type="int", name="LHS element")
   constraints.addColumn(type="int", name="RHS element")
   constraints.addColumn(type="str", name="ScatteringCrossSection")
   constraints.addColumn(type="str", name="State")
   constraints.addRow([0, 1, "2.*82.03/5.51", "eq"])

   VesuvioAnalysis(IPFile = "ip2018.par", ComptonProfile = table, AnalysisMode = "LoadReduceAnalyse",
                   NumberOfIterations = 2, OutputName = "polyethylene", Runs = "38898-38906", TOFRangeVector = [110.,1.5,460.],
                   Spectra = [135,182], MonteCarloEvents = 1e3, ConstraintsProfile = constraints, SpectraToBeMasked = [173,174,181],
                   SubtractResonancesFunction = 'name=Voigt,LorentzAmp=1.,LorentzPos=284.131,LorentzFWHM=2,GaussianFWHM=3;',
                   YSpaceFitFunctionTies = "(c6=0., c4=0.)")

   fit_results = mtd["polyethylene_H_JoY_sym_Parameters"]

   print("variable", "value")
   for row in range(fit_results.rowCount()):
       print(fit_results.column(0)[row],"{:.3f}".format(fit_results.column(1)[row]))

.. testcleanup:: ExVesuvioAnalysis

    config['default.facility'] = default_facility_orig
    config['default.instrument'] = default_instrument_orig

Output:

.. testoutput:: ExVesuvioAnalysis
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   variable value
   f1.sigma1 4.939
   f1.c4 0.000
   f1.c6 0.000
   f1.A 0.080
   f1.B0 0.000
   Cost function value ...

.. categories::

.. sourcelink::
