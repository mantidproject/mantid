.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is a refinement of the
:ref:`algm-FlatPlateAbsorption` algorithm for the specific
case of an HRPD 'slab can' sample holder. It uses the aforementioned
generic algorithm to calculate the correction due to the sample itself,
using numerical integration. This is done using the standard height x
width dimensions of an HRPD sample holder of 23 x 18 mm. Valid values of
the thickness are 2,5,10 & 15 mm, although this is not currently
enforced.

Further corrections are then carried out to account for the 0.125mm
Vanadium windows at the front and rear of the sample, and for the
aluminium of the holder itself (which is traversed by neutrons en route
to the 90 degree bank). This is carried out using an analytical
approximation for a flat plate, the correction factor being calculated
as
:math:`\rm{exp} \left( \frac{- \rho \left( \sigma_a \frac{ \lambda} {1.798} + \sigma_s \right) t}{\rm{cos} \, \theta} \right)`,
where :math:`\lambda` is the wavelength, :math:`\theta` the angle
between the detector and the normal to the plate and the other symbols
are as given in the property list above. The assumption is that the
neutron enters the plate along the normal.

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
:ref:`instrument <instrument>` associated with the workspace must be fully
defined because detector, source & sample position are needed.

ChildAlgorithms used
####################

The :ref:`algm-FlatPlateAbsorption` algorithm is used to
calculate the correction due to the sample itself.

Usage
-----

**Example:**

.. testcode:: ExSimpleHRPDSlab
    
    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    wsOut = HRPDSlabCanAbsorption (ws,Thickness='0.2',ElementSize=3)

    print "The created workspace has one entry for each spectra: %i" % wsOut.getNumberHistograms()

Output:

.. testoutput:: ExSimpleHRPDSlab

    The created workspace has one entry for each spectra: 1

.. categories::
