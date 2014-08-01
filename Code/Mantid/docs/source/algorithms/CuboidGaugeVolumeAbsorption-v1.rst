.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm uses a numerical integration method to calculate
attenuation factors resulting from absorption and single scattering
within a cuboid region of a sample with the dimensions and material
properties given. 

The gauge volume generated will be an axis-aligned
cuboid centred on the sample (centre) position. The sample must fully
enclose this cuboid. If this does not meet your needs you can instead
use the general :ref:`algm-AbsorptionCorrection`
algorithm in conjunction with
:ref:`algm-DefineGaugeVolume`.

Factors are calculated for each spectrum (i.e. detector position) and
wavelength point, as defined by the input workspace. The sample is
divided up into cuboids having sides of as close to the size given in
the ElementSize property as the sample dimensions will allow. Thus the
calculation speed depends linearly on the total number of bins in the
workspace and goes as :math:`\rm{ElementSize}^{-3}`.

Path lengths through the sample are then calculated for the centre-point
of each element and a numerical integration is carried out using these
path lengths over the volume elements.

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
:ref:`instrument <instrument>` associated with the workspace must be fully
defined because detector, source & sample position are needed. A sample
shape must have been defined using, e.g.,
:ref:`algm-CreateSampleShape` and the gauge volume must be
fully within the sample.


Usage
-----

**Example: A simple spherical sample with a cuboid guage volume**

.. testcode:: ExCuboidGuageSimpleSpere
    
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

    #restrict the number of wavelength points to speed up the example
    wsOut = CuboidGaugeVolumeAbsorption(ws, NumberOfWavelengthPoints=5, ElementSize=3,
        SampleHeight=1,SampleWidth=2,SampleThickness=3)

    print "The created workspace has one entry for each spectra: %i" % wsOut.getNumberHistograms()

Output:

.. testoutput:: ExCuboidGuageSimpleSpere

    The created workspace has one entry for each spectra: 1


.. categories::
