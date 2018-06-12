.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the scattering & transmission for Indirect Geometry spectrometers at
the elastic peak, as the transmission variation over wavelength is not
substantial.

The sample chemical formula is input for the :ref:`SetSampleMaterial
<algm-SetSampleMaterial>` algorithm to calculate the cross-sections. The
instrument analyser reflection is selected to obtain the wavelength of the
elastic peak to calculate the absorption cross-section. The sample mass
density/number density & thickness is input to then calculate the percentage scattering &
transmission.

Usage
-----

**Example - Running IndirectTransmission.**

.. testcode:: ExIndirectTransmissionSimple

    table_ws = IndirectTransmission(Instrument='IRIS', ChemicalFormula="C")
    param_names = table_ws.column(0)
    param_values = table_ws.column(1)

    for pair in zip(param_names, param_values):
      print("{} : {:1.10f}".format(pair[0], pair[1]))

Output:

.. testoutput:: ExIndirectTransmissionSimple

    Wavelength : 6.6580023372
    Absorption Xsection : 0.0129590747
    Coherent Xsection : 5.5510000000
    Incoherent Xsection : 0.0010000000
    Total scattering Xsection : 5.5520000000
    Number density : 0.0050139807
    Thickness : 0.1000000000
    Transmission (abs+scatt) : 0.9972136294
    Total scattering : 0.0027798910

**Example - Running IndirectTransmission with a specified number density and thickness.**

.. testcode:: ExIndirectTransmissionParams

    table_ws = IndirectTransmission(Instrument='OSIRIS', DensityType='Number Density', Density=0.5, Thickness=0.3, ChemicalFormula="C")
    param_names = table_ws.column(0)
    param_values = table_ws.column(1)

    for pair in zip(param_names, param_values):
      print("{} : {:1.10f}".format(pair[0], pair[1]))

Output:

.. testoutput:: ExIndirectTransmissionParams

    Wavelength : 6.6580023372
    Absorption Xsection : 0.0129590747
    Coherent Xsection : 5.5510000000
    Incoherent Xsection : 0.0010000000
    Total scattering Xsection : 5.5520000000
    Number density : 0.5000000000
    Thickness : 0.3000000000
    Transmission (abs+scatt) : 0.4339856278
    Total scattering : 0.5651699440

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
