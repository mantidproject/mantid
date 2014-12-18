.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a Monte Carlo simulation to calculate the
attenuation factor of a given sample for an arbitrary arrangement of
sample + container shapes. The algorithm proceeds as follows for each
spectra in turn:

-  A random point within the sample+container set up is selected and
   chosen as a scattering point;
-  This point then defines an incoming vector from the source position
   and an outgoing vector to the final detector;
-  The total attenuation factor for this path is then calculated as the
   product of the factor for each defined material of the
   sample/container that the track passes through.

Known limitations
-----------------

-  Only elastic scattering is implemented at the moment.

-  The object's bounding box is used as a starting point for selecting a
   random point. If the shape of the object is peculiar enough not to
   occupy much of the bounding box then the generation of the initial
   scatter point will fail.

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
:ref:`instrument <instrument>` associated with the workspace must be fully
defined because detector, source & sample position are needed.

At a minimum, the input workspace must have a sample shape defined.

Usage
-----

**Example: A simple spherical sample**

.. testcode:: ExMonteSimpleSample
    
    #setup the sample shape
    sphere = '''<sphere id="sample-sphere">
          <centre x="0" y="0" z="0"/>
          <radius val="0.1" />
      </sphere>'''

    ws = CreateSampleWorkspace("Histogram",NumBanks=1)
    ws = ConvertUnits(ws,"Wavelength")
    CreateSampleShape(ws,sphere)
    SetSampleMaterial(ws,ChemicalFormula="V")

    #Note: this is a quick and dirty evaluation, in reality you would need more than 300 events per point
    wsOut = MonteCarloAbsorption(ws,EventsPerPoint=300)

    print "The created workspace has one entry for each spectra: %i" % ws.getNumberHistograms()
    print "Just divide your data by the correction to correct for absorption."


Output:

.. testoutput:: ExMonteSimpleSample

    The created workspace has one entry for each spectra: 100
    Just divide your data by the correction to correct for absorption.

**Example: A simple spherical sample in a cylindrical container**

.. Ticket 9644 is in place to improve the python exports and expand this example

.. code-block:: python
    
   # The algorithm does allow you to set a complex sample environment
   # of different materials and shapes, but some of the required methods
   # are not exported to python yet.  This will come.


.. categories::
