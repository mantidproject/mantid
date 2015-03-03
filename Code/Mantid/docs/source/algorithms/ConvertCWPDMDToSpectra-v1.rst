.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithms is to collect the all the counts on the detectors among
a set of measurements, which belong to a same experiment run,
and bin them according to detector's position, i.e., :math:`2\theta`. 

This algorithm takes 2 MDEventWorkspaces as inputs.  
One stores the detectors' counts;
and the other stores the monitors' counts. 
These two MDEventWorkspaces are generated from algorithm ConvertSpiceToRealSpace. 

Futhermore, the unit of the output matrix workspace can be converted to 
d-spacing and momentum transfer (:math:`Q`). 


Input Workspaces
################

Two input MDEventWorkspaces that are required. 

{\it InputWorkspace} is an MDEventWorkspace stores detectors counts and sample logs. 
Each run in this MDEventWorkspace corresponds to an individual measurement point in experiment run. 
Each run has its own instrument object. 

The other input MDEventWorkspace, i.e., {\it InputMonitorWorkspace} contains the monitor counts of each measurement point.  
The signal value of each MDEvent in this workspace is the monitor counts
corresponding to an individual detector. 

These two MDEventWorkspace should have the same number of runs and same number of MDEvent.  


Outputs
#######

The output is a MatrixWorkspace containing the reduced powder diffraction data from a SPICE file for 
powder diffractometers in HFIR. 

Besides the data, it also has the sample logs' copied from the input workspace. 


Sample Logs
###########

The sample logs of the reduced HFIR powder diffraction data are aggregrated from all measurements points
in the experiment run. 

They are copied from the last ExperimentInfo object of the input MDWorkspace {\it InputWorkspace}. 


Target Units
############

Three units are supported by this algorithm.  They are :math:`2\theta`, dSpacing and MomentumTransfer(Q). 

The following equations are used to convert the units. 

.. math:: \lambda = 2d\sin(\theta)

.. math:: d = frac{4\pi}{Q}

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
.. math:: \sigma_i = f \times \sigma^{(n)}_i
where :math:`f` is the scaling factor. 

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


Workflow
--------

This algorithm is the third step to reduce powder diffraction data from a SPICE file.
Following algorithm {\it LoadSpiceAscii}, which loads SPICE file to a TableWorkspace
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

**Example - reduce a SPICE file for HB2A to Fullprof file:**

.. testcode:: ExReduceHB2AToFullprof

  # create table workspace and parent log workspace
  LoadSpiceAscii(Filename='HB2A_exp0231_scan0001.dat', 
        IntegerSampleLogNames="Sum of Counts, scan, mode, experiment_number",
        FloatSampleLogNames="samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass", 
        DateAndTimeLog='date,MM/DD/YYYY,time,HH:MM:SS AM', 
        OutputWorkspace='Exp0231DataTable', 
        RunInfoWorkspace='Exp0231ParentWS')

  # load for HB2A 
  ConvertSpiceDataToRealSpace(InputWorkspace='Exp0231DataTable', 
        RunInfoWorkspace='Exp0231ParentWS', 
        OutputWorkspace='Exp0231DataMD', 
        OutputMonitorWorkspace='Exp0231MonitorMD')

  # Convert from real-space MD to Fullprof data
  ConvertCWPDMDToSpectra(
        InputWorkspace = 'Exp0231DataMD',
        InputMonitorWorkspace = 'Exp0231MonitorMD',
        OutputWorkspace = 'Exp0231Reduced',
        BinningParams = '5, 0.1, 150',
        UnitOutput = '2theta',
        ScaleFactor = 100.,
        LinearInterpolateZeroCounts = True
        )

  # output
  ws = mtd["Exp0231Reduced"]
  
  vecx = ws.readX(0)
  vecy = ws.readY(0)
  vece = ws.readE(0)

  for i in [100, 100, 1101, 1228]:
    print "2theta = %-5f, Y = %-5f, E = %-5f" % (vecx[i], vecy[i], vece[i])

.. testcleanup::  ExReduceHB2AToFullprof

  DeleteWorkspace('Exp0231DataTable')
  DeleteWorkspace('Exp0231ParentWS')
  DeleteWorkspace('Exp0231DataMD')
  DeleteWorkspace('Exp0231MonitorMD')

Output:

.. testoutput:: ExReduceHB2AToFullprof

  Number of events = 2684

.. categories::
