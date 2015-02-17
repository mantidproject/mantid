.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the scattering & transmission for Indirect Geometry spectrometers at
the elastic peak, as the transmission variation over wavelength is not
substantial.

The sample chemical formula is input for the :ref:`SetSampleMaterial
<algm-SetSampleMaterial>` algorithm to calculate the cross-sections. The
instrument analyser reflection is selected to obtain the wavelength of the
elastic peak to calculate the absorption cross-section. The sample number
density & thickness is input to then calculate the percentage scattering &
transmission.

Usage
-----

**Example - Running IndirectTransmission.**

.. testcode:: ExIndirectTransmissionSimple

    table_ws = IndirectTransmission(Instrument='IRIS', ChemicalFormula="C")
    param_names = table_ws.column(0)
    param_values = table_ws.column(1)

    for pair in zip(param_names, param_values):
      print "%s : %s" % pair

Output:

.. testoutput:: ExIndirectTransmissionSimple

    Wavelength : 6.65800233718
    Absorption Xsection : 0.0129590747304
    Coherent Xsection : 5.551
    Incoherent Xsection : 0.001
    Total scattering Xsection : 5.552
    Number density : 0.1
    Thickness : 0.1
    Transmission (abs+scatt) : 0.945870519609
    Total scattering : 0.0540068963808

**Example - Running IndirectTransmission with a specified number density and thickness.**

.. testcode:: ExIndirectTransmissionParams

    table_ws = IndirectTransmission(Instrument='OSIRIS', NumberDensity=0.5, Thickness=0.3, ChemicalFormula="C")
    param_names = table_ws.column(0)
    param_values = table_ws.column(1)

    for pair in zip(param_names, param_values):
      print "%s : %s" % pair

Output:

.. testoutput:: ExIndirectTransmissionParams

    Wavelength : 6.65800233718
    Absorption Xsection : 0.0129590747304
    Coherent Xsection : 5.551
    Incoherent Xsection : 0.001
    Total scattering Xsection : 5.552
    Number density : 0.5
    Thickness : 0.3
    Transmission (abs+scatt) : 0.433985627752
    Total scattering : 0.565169943961

.. categories::
