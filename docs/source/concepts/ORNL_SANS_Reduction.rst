.. _Facilities File:

.. role:: xml(literal)
   :class: highlight
   
Reduction for ORNL SANS
=======================


This document explains how to use Mantid to perform reduction of ORNL SANS data.
Information about the underlying Mantid algorithms involved can be found in the 
:ref:`SANSReduction <algm-SANSReduction>` algorithm documentation.
For HFIR reduction specefically, you can also see the :ref:`HFIRSANSReduction <algm-HFIRSANSReduction>` algorithm documentation.


Contents

- `Reduction script`_

- `Reduction commands`_

 - `Beam center`_
 - `Dark current subtraction`_
 - `Normalization options`_
 - `Pixel masking`_
 - `Sensitivity correction`_
 - `Solid angle correction`_
 - `Transmission correction`_
 - `Absolute normalization`_
 - `Background subtraction`_
 - `I(Q) calculation`_


.. _`Reduction script`:

Reduction script
----------------

.. code-block:: python

    from reduction.instruments.sans.hfir_command_interface import *

    # You should call Clear() to re-initialize the Reducer
    Clear()

    # Start by declaring which instrument you are using
    HFIRSANS()

    # The following are setup options, they can appear in any order.
    # All files are assumed to be placed in the DataPath folder
    DataPath("../../../Test/Data/SANS2D/")
    DarkCurrent("BioSANS_dark_current.xml")
    DirectBeamCenter("BioSANS_empty_cell.xml")
    Mask(5,5,5,5)
    SetTransmission(0.51944, 0.011078)
    SaveIqAscii()

    # The following declares the file(s) that you will be reducing.
    AppendDataFile("BioSANS_test_data.xml")

    # Perform the reduction. This should be called last.
    Reduce1D()

The first two lines are optional and need only be used when you run the code outside of MantidPlot. The hfir_command_interface import statement gives us access the the various commands we will use to set up the reduction process.
The ``Clear()`` command re-initializes the reduction process.
The first important part of the script is to declare which instrument you are using. This will define the general flow of the reduction process. In this particular case, this is done by calling HFIRSANS().
The DataPath() command sets the directory where the data file will be found. Once this has been done, only the name of the data files need to be supplied to the various reduction commands.
The following commands are used to set options for the reduction process:

.. code-block:: python

    DarkCurrent("BioSANS_exp61_scan0000_0001.xml")
    DirectBeamCenter("BioSANS_exp61_scan0001_0015.xml")
    Mask(5,5,5,5)
    SetTransmission(0.51944, 0.011078)
    SensitivityCorrection("BioSANS_exp61_scan0031_0001.xml")
    SaveIqAscii()

Those commands do not need to be typed in any particular order. They only set options and define the reduction process that will be used later when processing each data file. See the list of commands for more details.

The ``AppendDataFile()`` command appends a data file to the list of files to be reduced. The reducer can process any number of data files, and the same reduction process will be applied to each of them.
The Reduce1D() command tell the reducer to start the reduction process. Since this command does the actual execution, it needs to be the last command in the reduction script.

.. _`Reduction commands`:

Reduction commands
------------------

The following is a list of reduction commands to apply corrections to the data and produce :math:`I(Q)`.


.. _`Beam center`:

Beam Center
^^^^^^^^^^^

Options for finding the beam center

``SetBeamCenter(x,y)``
    Sets the beam center location to be used, in pixel coordinates.

``DirectBeamCenter(datafile)``
   Finds the beam center using the direct beam method. The position of the beam center p is given by

        :math:`p(x,y) = \frac{\sum_i I_i \ d_i(x,y)}{\sum_i I_i}`

    where ``i`` runs over all pixels within the largest square detector area centered on the initial guess for the beam center position. The initial guess is the center of the detector. :math:`I_i` is the detector count for pixel ``i``, and :math:`d_i(x,y)` is the pixel coordinates. The calculation above is repeated iteratively by replacing the initial guess with the position found with the previous iteration. The process stops when the difference between the positions found with two consecutive iterations is smaller than 0.25 pixel.

``ScatteringBeamCenter(datafile, beam_radius=3.0)``
    Finds the beam center using the scattered beam method. The process is identical to the direct beam method, with the only difference being that the pixels within a distance R (the beam radius) of the beam center guess are excluded from the calculation. The direct beam is thus excluded and only the scattered data is used.

.. _`Dark current subtraction`:

Dark current subtraction
^^^^^^^^^^^^^^^^^^^^^^^^

``NoDarkCurrent()``
    Lets the reducer know that no dark current should be subtracted.

``DarkCurrent(datafile)``
    Specifies which data file to use for the dark current. The dark current is subtracted pixel by pixel by normalizing the dark current data by counting time, as follows:

        :math:`I'(x,y) = I_{data}(x,y) - \frac{T_{data}}{T_{dc}} \ I_{dc}(x,y)`

    where the T-values are the counting times for the data set and the dark current (dc).


.. _`Normalization options`:

Normalization options
^^^^^^^^^^^^^^^^^^^^^

``TimeNormalization()``
    Tells the reducer to normalize the data to counting time.

``MonitorNormalization()``
    Tells the reducer to normalize the data to the beam monitor.

``NoNormalization()``
    Tells the reducer not to normalize the data.


.. _`Pixel masking`:

Pixel masking
^^^^^^^^^^^^^

``Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0)``
    A band of pixels on each side of the detector is masked according to the input parameters.

``MaskRectangle(x_min, x_max, y_min, y_max)``
    Masks a rectangular region on the detector defined by the given pixel numbers.

``MaskDetectors(det_list)``
    Masks the given detector IDs.


.. _`Sensitivity correction`:

Sensitivity correction
^^^^^^^^^^^^^^^^^^^^^^

``SensitivityCorrection(flood_data, min_sensitivity=0.5, max_sensitivity=1.5)``
    The relative detector efficiency is computed the following way

        :math:`S(x,y) = \frac{I_{flood}(x,y)}{1/N_{pixels} \ \sum_{i,j} \ I_{flood}(i,j)}`

    where :math:`I_{flood}(x,y)` is the pixel count of the flood data in pixel (x,y). If a minimum and/or maximum sensitivity is given, the pixels having an efficiency outside the given limits are masked and the efficiency is recomputed without using those pixels.
    The sample data is then corrected by dividing the intensity in each pixels by the efficiency S

        :math:`I'_{sample}(x,y) = \frac{I_{sample}(x,y)}{S(x,y)}`

    The pixels found to have an efficiency outside the given limits are also masked in the sample data so that they don’t enter any subsequent calculations.

    If the user chose to use a dark current data set when starting the reduction process, that dark current data will be subtracted from the flood data. The subtraction is done before the sensitivity is calculated.

    If the user chose to use the solid angle correction for the reduction process, that correction will be applied to the flood data before the sensitivity is calculated.

    **Note**: The solid angle correction is either not applied at all, or applied to both the flood data to calculate the sensitivity correction and applied to the sample data as part of the reduction process.

``NoSensitivityCorrection()``
    Tells the reducer not to correct for detector sensitivity.

``SetSensitivityBeamCenter(x,y)``
    Sets the beam center for the flood data (may be different from the sample data).

``SensitivityDirectBeamCenter(datafile)``
    Tells the reducer to use the direct beam center finding method for the flood data (see DirectBeamCenter for details).

``SensitivityScatteringBeamCenter(datafile, beam_radius=3.0)``
    Tells the reducer to use the scattering beam center finding method for the flood data (see ScatteringBeamCenter for details).


.. _`Solid angle correction`:

Solid angle correction
^^^^^^^^^^^^^^^^^^^^^^

``SolidAngle()``
    Tells the reducer to apply the solid angle correction. The solid angle correction is applied as follows:

.. math::

    I'(x,y) = \frac{I(x,y)}{\cos^3(2\theta)}

    \sigma_{i'(x,y)} = \frac{\sigma_{I(x,y)}}{|\cos^3(2\theta)|}

``NoSolidAngle()``
    Tells the reducer not to apply the solid angle correction.

.. _`Transmission correction`:

Transmission correction
^^^^^^^^^^^^^^^^^^^^^^^

``SetTransmission(trans, error)``
    Sets the sample transmission. For each detector pixel, the transmission correction is applied as follows:

.. math::

    I'(x,y) = \frac{I(x,y)}{T^{[1+\sec(2\theta)]/2}}

    \sigma_{I'(x,y)} = \left[ \left[ \frac{\sigma_I}{T^{[1+\sec(2\theta)]/2}} \right]^2 + \left[ \frac{I(x,y)\sigma_T\left( \frac{1+\sec(2\theta)}{2}\right)}{T^{[\sec(2\theta)-1]/2}} \right]^2 \right]^{1/2}

``DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0)``
    Tells the reducer to use the direct beam method to calculate the sample transmission. The transmission is calculated as follows:

        :math:`T=\frac{\sum_{i; \ d(i,j)<R} \sum_j{\frac{I_{sample}(i,j)}{T_{sample}}}}{\sum_{i; \ d(i,j)<R} \sum_j{\frac{I_{beam}(i,j)}{T_{beam}}}}`

    where :math:`I_{sample}` and :math:`I_{beam}` are the pixel counts for the sample data set and the direct beam data set, respectively. The sums for each data set runs only over the pixels within a distance ``R=beam_radium`` of the beam center. :math:`T_{sample}` and :math:`T_{sample}` are the counting times for each of the two data sets. If the user chose to normalize the data using the beam monitor when setting up the reduction process, the beam monitor will be used to normalize the sample and direct beam data sets instead of the timer.
    
    If the user chose to use a dark current data set when starting the reduction process, that dark current data will be subtracted from both data sets before the transmission is calculated.

    Once the transmission is calculated, it is applied to the input data set in the same way as described for ``SetTransmission()``.

``BeamSpreaderTransmission(sample_spreader, direct_spreader, sample_scattering, direct_scattering, spreader_transmission=1.0, spreader_transmission_err=0.0 )``
    Tells the reducer to use the beam spreader ("glassy carbon") method to calculate the sample transmission. The transmission is calculated as follows:

        :math:`T=\frac{N_{gc, sample}/T_{gc, sample} - T_{gc}N_{sample}/T_{sample}}{N_{gc, empty}/T_{gc, empty} - T_{gc}N_{empty}/T_{empty}}`

    where :math:`N_{gc, sample}` and :math:`N_{gc, empty}` are the sums of all pixel counts for the sample and direct beam data sets with glass carbon, and :math:`N_{sample}` and :math:`N_{empty}` are the sums of all the pixel counts for the sample and direct beam without glassy carbon. The T values are the corresponding counting times. If the user chose to normalize the data using the beam monitor when setting up the reduction process, the beam monitor will be used to normalize all data sets instead of the timer.

    If the user chose to use a dark current data set when starting the reduction process, that dark current data will be subtracted from all data sets before the transmission is calculated.

    Once the transmission is calculated, it is applied to the input data set in the same way as described for ``SetTransmission()``.

``NoTransmission()``
    Tells the reducer not to apply a transmission correction.

``TransmissionDarkCurrent(dark_current)``
    Sets the dark current to be subtracted for the transmission measurement.

``ThetaDependentTransmission(theta_dependence=True)``
    If set to False, the transmission correction will be applied by dividing each pixel by the zero-angle transmission, without theta dependence.


.. _`Background subtraction`:

Background subtraction
^^^^^^^^^^^^^^^^^^^^^^

``Background(datafile)``
    The same reduction steps that are applied to the sample data are applied to the background data set. Those are the dark current subtraction, the data normalization, applying the detector mask, the sensitivity correction, the solid angle correction and the transmission correction. Although the same sensitivity correction is used for both sample and background, the background transmission is calculated separately from the sample transmission. Once all those reduction steps are applied to the background data set, the resulting background is subtracted from the sample data.

``NoBackground()``
    Tells the reducer not to subtract background.

``SetBckTransmission(trans, error)``
    Sets the background transmission.

``BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0)``
    Similar to ``DirectBeamTransmission``, this command sets the options to measure the background transmission.

``BckBeamSpreaderTransmission(sample_spreader, direct_spreader, sample_scattering, direct_scattering, spreader_transmission=1.0, spreader_transmission_err=0.0)``
    Similar to ``BeamSpreaderTransmission``, this command sets the options to measure the background transmission.

``BckTransmissionDarkCurrent(dark_current)``
    Similar to ``TransmissionDarkCurrent``, this command sets the dark current for the background.

``BckThetaDependentTransmission(theta_dependence=True)``
    Similar to ``ThetaDependentTransmission``, this command sets the theta-dependence option of the transmission correction for the background.

.. _`I(Q) calculation`:

I(Q) calculation
^^^^^^^^^^^^^^^^

``AzimuthalAverage(binning="0.01,0.001,0.11", suffix="_Iq", error_weighting=False, n_bins=100, log_binning=False)``
    Sets the options for azimuthal averaging. The binning parameter sets the binning of the output I(q) distribution in the following format: :math:`Q_{min}, \Delta Q, Q_{max}` (the binning will be found automatically if the ``binning`` parameter is not supplied). When letting the binning be calculated automatically, setting log_binning=True will tell the reducer to find the best log binning. The ``suffix`` parameter sets the suffix appended to the I(q) workspace name. If ``error_weighting`` is set to True, the pixel counts will be weighted by a function of the error when computing I(q) (see below).

    The binning of the output *I(Q)* distribution is defined by the user.
    It runs from :math:`Q_{min}` to :math:`Q_{max}` in steps of :math:`\Delta Q`.
    Each pixel is divided in :math:`N_{sub} \times N_{sub}` sub-pixels. Each sub-pixel is assigned a count equal to  of the original pixel count.

    The intensity I(Q) in each Q bin is given by

        :math:`I(Q_j) = \frac{1}{\sum_i \ w} \ \sum_i \ wI_i`

    where the sum runs over all sub-pixels *i* such that :math:`Q_j < q_i < Q_{j+1}`, where :math:`q_i` is the q-value of the given sub-pixel:

        :math:`q_i = \frac{4\pi \ \sin(\theta)}{\lambda}`

    The *w* factor is a weight that is set to 1 by default. Alternatively, pixels can be weighted as a function of their error by setting :math:`w=1/\Delta I_i`.

    The resolution in Q is computed using Mildner-Carpenter.

``IQxQy(nbins=100)``
    Option to produce the reduced I(Qx, Qy).

``NoIQxQy(nbins=100)``
    Turns off the option to produce the reduced I(Qx, Qy).

``NoSaveIq()``
    Tells the reducer not to save the output I(q).


Other
^^^^^

**Clear()**: Re-initializes the reducer. All options are set to default values.

**DataPath(path)**: Sets the directory containing all data files.

**Reduce1D()**: Tells the reducer to execute the reduction process.

**AppendDataFile(datafile, workspace=None)**: Appends a data file to the list of files to be reduced.

**SaveIqAscii()**: Tells the reducer to save the output I(q) to an ascii file. The file will have a name similar to the input file, with "_Iq" appended to it. The file will be located in the directory chosen with DataPath.


**SetSampleDetectorOffset(distance)**: Sets an additive sample-detector distance offset, in mm.

**SetSampleDetectorDistance(distance)**: Sets the sample-detector distance, in mm. If set, this distance will take priority over the distance found in the data file.

**SetWavelength(wavelength, spread)**: Sets the wavelength, in Angstrom. If set, this wavelength will take priority over the wavelength found in the data file.

**ResetWavelength()**: Resets the wavelength to the value found in the data file.


.. _`Absolute normalization`:

Absolute Normalization
^^^^^^^^^^^^^^^^^^^^^^

``SetAbsoluteScale(factor=1.0)``
    Sets a multiplicative scale factor to obtain I(Q) in absolute scale.

``SetDirectBeamAbsoluteScale(direct_beam, beamstop_radius=None, attenuator_trans=1.0, sample_thickness=None, apply_sensitivity=False)``
    Tells the reducer to use the direct beam method to compute the absolute scale factor. The direct_beam parameter is a valid file path to the direct beam data file. attenuator_trans is the attenuator transmission. The sample_thickness should be given in cm. If apply_sensitivity=True, the sensitivity correction will be applied to the direct beam data before the absolute scale factor is computed.

    The absolute cross-section in 1/cm is computed after all corrections including the transmission correction have been applied to the sample data. It is given by:
    
        :math:`d\Sigma/d\Omega = \frac{I(Q)}{KD}`

    where *D* is the sample thickness in *cm* and *K* is given by

        :math:`K=N \ \Delta\Omega`

    where *N* is the total empty beam detector counts per monitor count divided by the attenuation factor at the used wavelength, and :math:`\Delta\Omega` is the square of the ratio of the pixel size to the sample-detector distance.
