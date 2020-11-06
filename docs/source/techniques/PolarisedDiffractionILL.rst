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
   config.appendDataSearchSubDir('ILL/D7/')

.. testcode:: BasicReduction

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
    sAxis = SofQ[0].readY(0)  # S axis
    print('S(Q): S range: {:.2}...{:.2} barn/sr/formula unit'.format(np.min(sAxis), np.max(sAxis)))

Output:

.. testoutput:: BasicReduction

    S(Q): S range: 0.016...0.06 barn/sr/formula unit

   

Wavelength and position calibration
===================================

The first step of working with D7 data is to ensure that there exist a proper calibration of the wavelenght, bank positions, and detector positions relative to their bank. This calibration can be either taken from a previous experiment performed in comparable conditions or obtained from the :math:`\text{Y}_{3}\text{Fe}_{5}\text{O}_{12}` (YIG) scan data with a dedicated algorithm :ref:`D7YIGPositionCalibration <algm-D7YIGPositionCalibration>`. The method follows the description presented in Ref. [1].

This algorithm performs wavelength and position calibration for both individual detectors and detector banks using measurement of a sample of powdered YIG. This data is fitted with Gaussian distributions at the expected peak positions. The output is an :ref:`Instrument Parameter File <InstrumentParameterFile>` readable by the :ref:`LoadILLPolarizedDiffraction <algm-LoadILLPolarizedDiffraction>` algorithm that will place the detector banks and detectors using the output of this algorithm.

The provided YIG d-spacing values are loaded from an XML list. The default d-spacing distribution for YIG is coming from Ref. [2]. The peak positions are converted into :math:`2\theta` positions using the initial assumption of the neutron wavelength. YIG peaks in the detector's scan are fitted separately using a Gaussian distribution.

The workspace containing the peak fitting results is then fitted using a `Multidomain` function of the form:

.. math:: 2\theta_{fit} = m \cdot (2.0 \cdot \text{asin} ( \lambda / 2d ) + offset_{\text{pixel}} + offset_{\text{bank}}),

where `m` is the bank slope, :math:`offset_{\text{pixel}}` is the relative offset to the initial assumption of the position inside the detector bank, and :math:`offset_{\text{bank}}` is the offset of the entire bank. This function allows to extract the information about the wavelength, detector bank slopes and offsets, and the distribution of detector offsets.


**Example - D7YIGPositionCalibration - calibration at the shortest wavelength**

.. code-block:: python

   approximate_wavelength = '3.1' # Angstrom
   D7YIGPositionCalibration(Filenames='402652:403041', ApproximateWavelength=approximate_wavelength,
                               YIGPeaksFile='D7_YIG_peaks.xml', CalibrationOutputFile='test_shortWavelength.xml',
                               MinimalDistanceBetweenPeaks=1.5, BankOffsets="-3,-3,1", ClearCache=True,
                               FitOutputWorkspace='shortWavelength')

   print('The calibrated wavelength is: {0:.2f}'.format(float(approximate_wavelength)*mtd['shortWavelength'].column(1)[1]))
   print('The bank2 gradient is: {0:.3f}'.format(1.0 / mtd['shortWavelength'].column(1)[0]))
   print('The bank3 gradient is: {0:.3f}'.format(1.0 / mtd['shortWavelength'].column(1)[176]))
   print('The bank4 gradient is: {0:.3f}'.format(1.0 / mtd['shortWavelength'].column(1)[352]))


	     
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


#. T. Fennell, L. Mangin-Thro, H.Mutka, G.J. Nilsen, A.R. Wildes.
   *Wavevector and energy resolution of the polarized diffuse scattering spectrometer D7*,
   Nuclear Instruments and Methods in Physics Research A **857** (2017) 24–30
   `doi: 10.1016/j.nima.2017.03.024 <https://doi.org/10.1016/j.nima.2017.03.024>`_

#. A. Nakatsuka, A. Yoshiasa, and S. Takeno.
   *Site preference of cations and structural variation in Y3Fe5O12 solid solutions with garnet structure*,
   Acta Crystallographica Section B **51** (1995) 737–745
   `doi: 10.1107/S0108768194014813 <https://doi.org/10.1107/S0108768194014813>`_


.. categories:: Techniques
