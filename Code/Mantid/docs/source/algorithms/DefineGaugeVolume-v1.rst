.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Intended for use on data from engineering beamlines, this algorithm
creates a shape object for use as the 'gauge volume' (i.e. the portion
of the sample that is visible to the detectors in a given run) of a
larger sample in the :ref:`algm-AbsorptionCorrection`
algorithm. The sample shape will also need to be defined using, e.g.,
the :ref:`algm-CreateSampleShape` algorithm. Shapes are
defined using XML descriptions that can be found
:ref:`here <HowToDefineGeometricShape>`.

Internally, this works by attaching the XML string (after validating it)
to a property called "GaugeVolume" on the workspace's :ref:`Run <Run>`
object.


Usage
-----

**Example: A simple spherical sample with a cuboid guage volume**

.. testcode:: ExSimpleSpereWithCuboidGuage
    
    #setup the sample shape
    sphere = '''<sphere id="sample-sphere">
          <centre x="0" y="0" z="0"/>
          <radius val=".2" />
      </sphere>'''
    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    CreateSampleShape(ws,sphere)
    SetSampleMaterial(ws,ChemicalFormula="V")

    #setup the guage volume
    cuboid = '''<cuboid id="shape">
        <left-front-bottom-point x="0.01" y="-0.1" z="0.0"  />
        <left-front-top-point  x="0.01" y="-0.1" z="0.02"  />
        <left-back-bottom-point  x="-0.01" y="-0.1" z="0.0"  />
        <right-front-bottom-point  x="0.01" y="0.1" z="0.0"  />
      </cuboid>'''
    DefineGaugeVolume(ws,cuboid)

    #restrict the number of wavelength points to speed up the example
    wsOut = AbsorptionCorrection(ws, NumberOfWavelengthPoints=5, ElementSize=2)

    print "The created workspace has one entry for each spectra: %i" % wsOut.getNumberHistograms()

Output:

.. testoutput:: ExSimpleSpereWithCuboidGuage

    The created workspace has one entry for each spectra: 1

.. categories::
