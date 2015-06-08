.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithms is to convert an experiment done on reactor-based four-circle instrument
(such as HFIR HB3A) to a MDEventWorkspace with each MDEvent in momentum space. 


In this algorithm's name, ConvertCWSDToMomentum, *CW* stands for constant wave 
(reactor-source instrument); *SD* stands for single crystal diffraction.

This algorithm takes ??? as inputs.

Futhermore, the unit of the output matrix workspace can be converted to 
momentum transfer (:math:`Q`). 


Outline of algorithm
####################

1. Create output workspace.
 * Build a virtual instrument, requiring  
   - position of source
   - position of sample
   - detector ID, position, detector size of pixels
2. Read in data via table workspace
 * From each row, (1) file name and (2) starting detector ID are read in.  
 * Detector position in (virtual) instrument of MDEventWorkspace is compared with the position in MatrixWorkspace 
 * Momentum is calcualted by goniometry values


Input Workspaces
################

Two TableWorkspaces, which contain experiment information, are required. 

**InputWorkspace** is a TableWorkspace containing the data files names to be loaded for the experiment. 
It is required to have 4 columns.  
They are *Scan*, *Pt*, *Filename* and *StartDetID* respectively. 

A typical HB3A experiment consists of multiple scans, each of which contains multiple measurement point (i.e., Pt). 
*FileName* is the XML data file for 2D detector information for a specific Scan/Pt pair. 
*StartDetID* is the starting detector ID of a specific Scan/Pt mapped to the virtual instrument. 

**DetectorTableWorkspace** is the TableWorkspace 


Outputs
#######

The output is an MDEventWorkspace ... 


MDEvent
+++++++

Each MDEvent in output MDEventWorkspace contain 
* *Kx*
* *Ky*
* *Kz*
* Signal
* Error
* Detector ID
* Run Number



Combine Experiment Into One MDEventWorkspace
--------------------------------------------

One typical HB3A (reactor-based four-circle diffractometer) experiment consists of 
a set of scans, each of which contains multiple experiment point (labeled as *Pt.* in SPICE). 

Each experiment point is independent to the others. 
They can have various detector positions, goniometer setup and even sample environment setup.

In order to integrate them into an organized data structure, i.e., *MDEventWorkspace*, 
a virtual instrument is built in the algorithm. 

Virtual instrument
==================

A virtual instrument is built in the algorithm. 
In this virtual instrument, the number of detectors and their position are determined 
by the number of individual detector's positions in the *experiment*.


MDEventWorkspace
================

There is only one *virtual* instrument and *N* ExperimentInfo.  
*N* is the total number of experiment points in the *experiment*. 


Sample Logs
###########



Target Units
############

Three units are supported by this algorithm via property *UnitOutput*.  
They are :math:`2\theta`, dSpacing and MomentumTransfer(Q). 

The following equations are used to convert the units. 

.. math:: \lambda = 2d\sin(\theta)

.. math:: d = \frac{4\pi}{Q}

Therefore neutron wavelength :math:`\lambda` must be given either in sample log or via input property
if the unit of the output workspace is targeted to be dSpacing or MomentumTransfer. 


Binning, Normalization and Error
################################

According to the input binning parameters, the bins in :math:`2\theta` are created as
:math:`2\theta_{min}, 2\theta_{min}+\Delta, 2\theta_{min}+2\Delta, \cdots`. 

If the unit of ouput workspace is specified as dSpacing or MomentrumTransfer,
then the bins should be created as :math:`d_{min}, d_{min}+\Delta, d_{min}+2\Delta, \cdots, d_{max}`
or :math:`q_{min}, q_{min}+\Delta, \cdots, q_{max}` respectively. 

For each detector, if its position falls between :math:`2\theta_i` and :math:`2\theta_{i+1}`,,
:math:`d_i` and :math:`d_{i+1}`, or :math:`Q_i` and :math:`Q_{i+1}`, 
then its counts is added to :math:`Y_i` and the corresponding monitor counts is added to 
:math:`M_i`. 

The singals on these bins are normalized by its monitor counts, such that 

.. math:: y_i = \frac{Y_i}{M_i}


The error (i.e., standard deviation) is defined as 

.. math:: \frac{\Delta y_i}{y_i} = \sqrt{(\frac{\Delta Y_i}{Y_i})^2 + (\frac{\Delta M_i}{M_i})^2}

Scaling
#######

The normalized histogram can be scaled up by a factor specified by  *ScaleFactor*. 
In most cases, the scaling factor is equal to average monitor counts of all measurements. 

If the scaling factor is specified, then
the standard error of data point :math:`i` will be converted to 

.. math:: \sigma^{(s)}_i = f \times \sigma^{(n)}_i

where :math:`f` is the scaling factor, :math:`\sigma^{(n)}_i` is the standard error of the normalized signal
of data point :math:`i`, and 
:math:`\sigma^{(s)}_i` is the standard error of the signal scaled up. 

Linear Interpolation
####################

If a user specifies a bin size that is smaller than the resolution of the instrument, 
then it is very likely to occur that some bins have zero count, while their neighboring
bins have counts that are significantly larger than noise. 
In this case, an option to do linear interpolation to the zero count bins 
in the histogram is provided. 
Property *LinearInterpolateZeroCounts* is used to set the flag to do linear interpolation. 

The linear interpolation will be only applied to those zero-count bins within
the measuring range. 

Excluding detectors
###################

Detectors can be excluded from conversion process.  
They can be specified by their *Detector ID*s via property *ExcludedDetectorIDs*.  
If a detector is specified as being excluded, 
all of its counts of all runs (pts) will be taken out of binning process. 


Workflow
--------

This algorithm is the third step to reduce powder diffraction data from a SPICE file.
Following algorithm *LoadSpiceAscii*, which loads SPICE file to a TableWorkspace
and {\it ConvertSpiceToRealSpace}, which converts the TableWorkspace to MDEvnetWorkspace 
that is able to reflect all the information of the epxeriment,
{\it ConvertCWPDMDToSpectra} goes through all the detectors' counts and rebins the data. 

An Example
##########

1. LoadSpiceAscii
2. ConvertSpiceToRealSpace
3. Merge a few data MDWorkspaces together; merge the corresponding monitor MDWorkspaces together;
4. ConvertCWPDMDToSpectra. 

Experimental data with different neutron wavelengths can be binned together to d-spacing or momentum transfer space. 


Usage
-----

**Example - convert SPICE file for HB3A to Fullprof file:**

.. testcode:: ExReduceHB2AToFullprof

  # create table workspace and parent log workspace
  LoadSpiceAscii(Filename='HB2A_exp0231_scan0001.dat', 
        IntegerSampleLogNames="Sum of Counts, scan, mode, experiment_number",
        FloatSampleLogNames="samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass", 
        DateAndTimeLog='date,MM/DD/YYYY,time,HH:MM:SS AM', 
        OutputWorkspace='Exp0231DataTable', 
        RunInfoWorkspace='Exp0231ParentWS')


  # output

.. testcleanup::  ExReduceHB2AToFullprof

  DeleteWorkspace('Exp0231DataTable')

Output:

.. testoutput:: ExReduceHB2AToFullprof

  2theta = 15.000000, Y = 0.386563, E = 0.024744

.. categories::
