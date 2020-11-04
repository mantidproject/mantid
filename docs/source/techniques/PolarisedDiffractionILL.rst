.. _PolarisedDiffractionILL:

======================================
Data reduction for ILL's D7 instrument
======================================

.. contents:: Table of contents
    :local:

There are three workflow algorithms supporting data reduction at ILL's D7 polarised diffraction and spectroscopy instrument. These algorithms are:

:ref:`algm-D7YIGPOsitionCalibration`
    Performs wavelength and position calibraiton for D7 instrument banks and individual detectors in banks. 

:ref:`algm-PolDiffILLReduction`
    Performs data reduction and produces the unnormalised sample cross-sections in one of the available units.

:ref:`algm-D7AbsoluteCrossSections`
    Performs cross-section separation into nuclear coherent, spin-incoherent and magnetic components, and does the data normalisation.

Together with the other algorithms and services provided by the Mantid framework, the reduction algorithms can handle a number of reduction scenarios. If this proves insufficient, however, the algorithms can be accessed using Python. Before making modifications it is recommended to copy the source files and rename the algorithms as not to break the original behavior.

This document tries to give an overview on how the algorithms work together via Python examples. Please refer to the algorithm documentation for details of each individual algorithm.


Reduction workflow and recommendations
######################################

A description of the usage of the algorithms for the D7 data reduction is presented along with several possible workflows, depending on the number of desired corrections and the type of normalisation.

Reduction basics
================

.. include:: ../usagedata-note.txt

A very basic reduction would include a vanadium reference and a sample, without any corrections or position and wavelength calibration, and follow the steps:

#. Reduce vanadium data.

#. Reduce sample data.

#. Run normalisation with vanadium reduction output as input.

.. testsetup:: BasicReduction

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D7'

.. testcode:: BasicReduction

    # Uncomment to add a temporary data search directory.
    #mantid.config.appendDataSearchDir('path/to/data/')

    # Define vanadium properties:
    vanadiumProperties = {'FormulaUnits': 50}
    
    # Vanadium reduction
    PolDiffILLReduction(Run='396993', ProcessAs='Vanadium', OutputWorkspace='reduced_vanadium',
                        SampleAndEnvironmentProperties=vanadiumProperties)

    # Define the number of formula units for the sample
    sampleProperties = {'FormulaUnits': 182.54}
    # Sample reduction
    PolDiffILLReduction(Run='397004', ProcessAs='Sample', OutputWorkspace='reduced_sample',
                            SampleAndEnvironmentProperties=sampleProperties)

    # normalise sample and set the output to absolute units with vanadium
    D7AbsoluteCrossSections(InputWorkspace='reduced_sample', OutputWorkspace='normalised_sample',
                            FormulaUnits=sampleProperties['FormulaUnits'],
                            NormalisationMethod='Vanadium', VanadiumInputWorkspace='reduced_vanadium')

    SofQ = mtd['normalised_sample']
    qAxis = SofQ[0].readX(0)  # Q axis
    print('S(Q): Q range: {:.3}...{:.3}A'.format(qAxis[0], qAxis[-1]))

Output:

.. testoutput:: BasicReduction

    S(Q): Q range: 0.455...3.781A

   

Wavelength and position calibration
===================================

.. include:: ../usagedata-note.txt



	     
Transmission calculation
========================


Polarisation correction
=======================

Sample data reduction
=====================

Sample data normalisation
-------------------------

#. Vanadium normalisation

#. Paramagnetic normalisation

#. Spin-incoherent normalisation



.. categories:: Techniques
