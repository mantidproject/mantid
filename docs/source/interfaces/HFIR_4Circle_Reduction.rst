HFIR Single Crystal Reduction Interface
=======================================

.. contents:: Table of Contents
  :local:

Overview
--------

HFIR single crystalreduction interface is a GUI to download, view and reduce data from
HFIR's four-circle single crystal diffractometer in SPICE format.


Introduction of Tabs
--------------------

1.  **Setup and Data Access**: Configure the instrument name, data server URL and directories.

    - Configure the instrument name;
    - Set up and test HB3A data server's URL;
    - Configure the directory to save raw data;
    - Configure the directory to save working result;
    - Download data from server;


2.  **View Raw Data**: View 2D image of counts on detector of one measurement.

    - Plot the counts of the 256 by 256 2D detector;


3.  **Calculate UB**: Calculate UB matrix.

    - Find peak in one measurement;
    - Option to load Miller index directly from SPICE file;
    - Calculate UB matrix;
    - Re-index the peaks;
    - Refine UB matrix with more than two peaks.


4.  **Merge Scan**: Merge all the measurements in a scan.

    - Merge all measuring points in a scan to an MDEventWorkspace in HKL-frame or Q-sample-frame;
    - Allow various ways to set up UB matrix


5.  **Peak Integration**: Integrate peaks

    - Still in developmenet


6.  **Survey**: Get experiments runs' information by scanning through all SPICE files in an experiment

    - For selected scans in an experiment, list the run (i.e., Pt.) with maximum detector counts in a specific scan;
    - For each listed run, the information show includes detector counts and HKL



Use Cases
---------

Here are some use cases that can be used as examples.

Set up reduction
++++++++++++++++

The first step to reduce data with 4-Circle GUI is to set up the reduction environment.
Reduction cannot work correctly if this step is **SKIPPED**.

1.  Go to tab *Setup & Data Access*;
2.  Set up *Data Directory* and *Working Directory*;
3.  An alternative way to step 2 is to push button *Load Default*;
4.  Push button *Apply*.  Mantid thus checks whether the setup, including source data file's URL, data directory and working directory are valid. Be noticed that the check of URL may return **timeout** error.  It might be a false alarm.  You might push the *Apply* button few more times.


Experiment overview
+++++++++++++++++++

Usually if it is the first time to reduce data for an experiment, it is
recommended to go through the whole experiment to gather some information.

1.  Go to tab *Survey*;
2.  Set up experiment number on the top of application window;
3.  Input the range of the scan numbers for survey. Leaving them blank will result in going through all scans of the experiment;
4.  Click button *Survey*;
5.  It might take a while to load and scan all the SPICE files belonged to an individual experiment.  After it is finished, the result will be shown in the table.
6.  Save the survey result for future by clicking button *Save*;
7.  Select a row, and click button *View Peak*, application will switch to tab *View Raw Data* automatically and set the scan and Pt number from the selected row.


Workflow to calculate and refine UB matrix
++++++++++++++++++++++++++++++++++++++++++

Here is a typical use case to calculate UB matrix after initial setup.

1.  User specifies *Experiment* and pushes button *Set*
2.  Users may do a new survey or load a survey result file in tab *Survey*;
3.  User enters tab *View Raw Data* and inputs scan number and list all the measuring points (Pt.)
4.  User views all the measurements  

    *  User finds out the measurement with the strongest reflection and push button use
    *  Alternatively, user can use the survey result to find out the Pt. with the maximum counts of the scan
 
5.  GUI shifts to tab *Calculate UB* automatically
6.  User pushes button *Find Peak* with checking *Load HKL from file*
7.  GUI finds the peak center and load HKL
8.  User pushes button *Add peak* to add the peak to table
9.  User repeats step 2 to 9 to add other peaks
10.  User select the peaks that are linearly independent and pushes *Calcualte UB*
11.  GUI calculates UB matrix and show the result
12.  User may push *Index peak* to use the calculated UB matrix to index peaks in the table to check UB matrix;
13.  User may refine the UB matrix and thus lattice parameters
 
     a. user adds more peaks to the UB peak table;
     b. user selects at least 3 non-degenerate peaks;
     c. user clicks button *Refine*;
     d. application refines UB matrix and outputs the refined UB matrix, refined lattice parameters and their error. 


Workflow to merge measurements in scan
++++++++++++++++++++++++++++++++++++++

Here is a typical use case to merge all the measuring points (Pt.) in a scan

1.  User specifies *Experiment* and pushes button *Set*

2.  User enters tab *Merge Scan*

3.  User specifies the UB matrix either by *From tab Calculate UB* or by entering the values to text editor

4.  User pushes button *Set*

5.  User specifies the frame in which the merged data will be in. If the target frame is Q-Sample-Sapce, then there is no need to specify UB matrix

6.  User specifies the scan numbers and push button *Add*

7.  User specifies the base name for the output MDEventWorkspaces

8.  User pushes button *Process*

9.  User goes to MantidPlot to view the merged scan by SliceView or Vates.


Peak Integration with automatic background subtraction by approximation
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This is an easy approximation to integrate a peak with background subtraction
by specified region of interest.

The pre-requisit is that all events are normalized by monitor counts
and scaled up by same factor (e.g, 1500).

1.  Set up region of interest

    - define region of interest on the detector
    - check whether the ROI does not exclude any peak

2.  Specify background Pts.

    - specify the Pts. in the scan that are used to estimate background

3.  Integrate peaks

    -  select peaks to integrate
    -  integrate peaks

4.  Review the integration result
5.  Export to Fullprof peak integration (.int) file.



UB Matrix Calcualtion and Refinement
------------------------------------


Converting SPICE UB matrix to Mantid UB matrix
++++++++++++++++++++++++++++++++++++++++++++++

Assuming that SPICE UB matrix (3 x 3) is composed as
 * R11, R12, R13
 * R21, R22, R23
 * R31, R32, R33

Then, converted to UB matrix in Mantid, it is like
 *  R11,  R12,  R13
 *  R31,  R32,  R33
 * -R21, -R22, -R23


Peak Integration
----------------

Unlike TOF single crystal diffractometer, HB3A uses a different method to integrate diffraction peaks.

Presently, HB3A reduction interface supports 3 types of integrations.

Normalization
+++++++++++++

Counts of neutron on any detector shall be normalized by its corresponding monitor
count and then be multiplied by a constant specified by user.

But for HB3A, the normalization to counting *time* is more reliable because the
beam monitor may be unstable.


Region of interest
++++++++++++++++++

A region of interest (ROI) on the detector can be defined for each scan.
The signals in the ROI of each Pt. in a scan construct a 3-dimensional diffraction peak.

The purpose to define the region of interest on detector is to reduce the affect of background noise.


Integration algorithm 1: simple counts summation
++++++++++++++++++++++++++++++++++++++++++++++++

This algorithm is also called *simple cuboid integration*,
which is to approximate the integrated peak intensity.

Measuring one peak usually contains around 20 Pt. in a same scan.
In most of the cases, the first and last several measurements (called as *Pt* in SPICE) are
background.
Therefore, the background for per measurement can be estimated by averaging the
summed number of counts normalized by either monitor counts or measuring time.

The integrated peak intensity is

.. math:: I = \sum_i (C_i - B_i) \times \Delta X

where
  * :math:`C_i` is the normalized detector counts in ROI of measurement *i*
  * :math:`\Delta X` is the motor step
  * :math:`B_i` is the estimated background

The error can be calculated as

.. math:: \sigma = \sum_i \sqrt{C_i} \cdot \Delta X


Estimating background
^^^^^^^^^^^^^^^^^^^^^

For each measurment, the background :math:`B_i` is calculated as

.. math:: B^{(e)} = \frac{\sum_i^{<pt>}C_i}{|<pt>|}

where :math:`<pt>` is a set of measurment points that are specified by users.
Usually they are the first and last several measurements in a scan.

Then this estimated **normalized** background value can be applied to each measuremnt, whose counts are normalized.


Integration algorithm 2: simple counts summation with fitted background
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This algorithm is based on previous algorithm.
It is assumed that if the statistic of the diffraction peak is good enough, then
the curve, i.e., moving motor position against normalized counts, can be
fitted with a Gaussian plus flat background.

.. Gaussian formula comes from http://mathworld.wolfram.com/GaussianFunction.html

.. math:: C = A\times e^{-(x - x_0)^2/(2s^2)} - B

where 
  * *x* is the (moving) motor position
  * *C* is the normalized counts in ROI when the moving motor is at *x*

The integrated peak intensity and its error will be calculated as

.. math:: I = \sum_i^{<pt>} (C_i - B) \times \Delta X 

where
  *  :math:`C_i` is the normalized detector counts in ROI of measurement *i* 
  *  :math:`\Delta X` is the motor step
  *  :math:`B_i` is the estimated background
  *  the set of measurements *<pt>* is defined by the motor positions in the range of :math:`x_0 \pm \frac{N}{2}FWHM`.

     -  usually the default value of *N* is set to 2.
     -  :math:`FWHM = 2\sqrt{2\ln2}s \approx 2.3548s`

The error can be calculated as

.. math:: \sigma = \sum_i \sqrt{C_i} \cdot \Delta X


Integration algorithm 3: calculate intensity from fitted model
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

It is assumed that for a well measured diffraction peak, in 3D, 
the counts in ROI of each measurement from the edge of the peak to the other edge of peak 
against the moving motor's positions should be represented by a Gaussian function with 
flat background

.. math:: C = A\times e^{-(x - x_0)^2/(2s^2)} - B

Then the peak intensity should be the integral of the Gaussian from :math:`-\inf` to :math:`+\inf`,
i.e., 

.. math:: I = A\times s\times\sqrt{2\pi}

The error of the intensity should be calculated by the propagation of fitted error of *A* and *s*.

.. math:: \sigma_I^2 = 2\pi (A^2\cdot \sigma_s^2 + \sigma_A^2\cdot s^2 + 2\cdot A\cdot s\cdot \sigma_{As})

Issue
^^^^^

It is found that the standard deviation of *A* from covariance matrix calculated from **scipy.curve_fit** library
is very large, which causes an unreasonably large estimated error on integrated intensity *I*.


Other peak integration algorithms in consideration
++++++++++++++++++++++++++++++++++++++++++++++++++

There are some other peak integration algorithms that we discussed.
None of them has been implemented.
But it is still worth to document them here.


Peak Integration with automatic background subtraction by IntegrateEllipsoids
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is no existing algorithm in Mantid to integrate ellipsoid because1
algorithm *IntegrateEllipsoids* works only for event in unit as time-of-flight.

So far, there is only one algorithm is implemented to integrate peaks for HB3A.


Integrating a peak in cuboid in Q-space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the Q-space, by masking each measurement, it is assumed that the peak's intensity
is very close to the number of counts in the unmasked cuboid normalized either by
measuring time or beam monitor counts with background removed.

.. math:: I = \sum_{i} \frac{n_i}{F_i} - B^{(e)}

where :math:`n_i` is the counts of Pt i in the region of interest,
:math:`F_i` is the normalization factor of Pt i,
and `B^{(e)}` is the estimated background per Pt with the same
normalization type of :math:`F_i`.


Estimating background
^^^^^^^^^^^^^^^^^^^^^

For each measurment, the background :math:`B_i` is calculated as

.. math:: B_i = \frac{\sum^{(pt)}_{\{d_i\}}n_{d_i}}{F^{(a)}_{d_i}}

where :math:`F^{(a)}` is the normlization of either time or beam monitor counts,
and :math:`n_{d_i}` is the neutron counts of measumrent :math:`d_i`.

Then the estimation of the normalized background for each measurement is

.. math:: B^{(e)} = \sum_{\{P_i\}}\frac{B_i}{N}

where :math:`N` is the number of measurements used to calculated background.



.. categories:: Interfaces
