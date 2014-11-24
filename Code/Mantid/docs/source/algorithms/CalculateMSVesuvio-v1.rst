.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the multiple scattering contribution for deep inelastic neutron scattering on
the `Vesuvio <http://www.isis.stfc.ac.uk/instruments/vesuvio/vesuvio4837.html>`__ instrument at
ISIS. The algorithm follows the procedures defined by J. Mayers et al. [1]_.


Usage
-----

.. code-block:: python

   runs = "" # fill in run numbers here
   ip_file = "" # fill in IP file here
   data = LoadVesuvio(Filename=, SpectrumList=spectra,
                      Mode="SingleDifference", InstrumentParFile=ip_file)
   # Cut it down to the typical range
   data = CropWorkspace(raw_ws,XMin=50.0,XMax=562.0)
   # Coarser binning
   data = Rebin(raw_ws, Params=[49.5, 1.0, 562.5])

   # Create sample shape
   height = 0.1 # y-dir (m)
   width = 0.1 # x-dir (m)
   thick = 0.005 # z-dir (m)
   
   half_height, half_width, half_thick = 0.5*height, 0.5*width, 0.5*thick
   xml_str = \
      " <cuboid id=\"sample-shape\"> " \
       + "<left-front-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width,-half_height,half_thick) \
       + "<left-front-top-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, half_height, half_thick) \
       + "<left-back-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, -half_height, -half_thick) \
       + "<right-front-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (-half_width, -half_height, half_thick) \
       + "</cuboid>"
   CreateSampleShape(data, xml_str)
   atom_props = [1.007900, 0.9272392, 5.003738,
                 16.00000, 3.2587662E-02, 13.92299,
                 27.50000, 4.0172841E-02, 15.07701]
   tot_scatter, ms_scatter = \
       CalculateMSVesuvio(data, NoOfMasses=3, SampleDensity=241, AtomicProperties=atom_props,
                          BeamRadius=2.5)

References
----------

.. [1] J. Mayers, A.L. Fielding and R. Senesi, `Nucl. Inst Methods A 481, 454(2002) <http://dx.doi.org/10.1016/S0168-9002(01)01335-3>`__



.. categories::
