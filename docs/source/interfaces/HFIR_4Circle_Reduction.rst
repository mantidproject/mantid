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

  1. **Setup and Data Access**: Configure the instrument name, data server URL and directories.
  
    - Configure the instrument name;
    - Set up and test HB3A data server's URL;
    - Configure the directory to save raw data;
    - Configure the directory to save working result;
    - Download data from server;
    
  2. **View Raw Data**: View 2D image of counts on detector of one measurement.
  
    - Plot the counts of the 256 by 256 2D detector;
    
  3. **Calculate UB**: Calculate UB matrix.
  
    - Find peak in one measurement;
    - Option to load Miller index directly from SPICE file;
    - Calculate UB matrix;
    - Re-index the peaks;
    
  4. **Merge Scan**: Merge all the measurements in a scan.
  
    - Merge all measuring points in a scan to an MDEventWorkspace in HKL-frame or Q-sample-frame;
    - Allow various ways to set up UB matrix
    
  5. **Refine UB**: Refine UB matrix
  
    - Disabled becaues it is in development still;
      
  6. **Peak Integration**: Integrate peaks
  
    - Disabled because it is still in development.

Use Cases
---------

Here are some use cases that can be used as examples.


Workflow to calculate UB matrix
+++++++++++++++++++++++++++++++

Here is a typical use case to calculate UB matrix

 1. User specifies *Experiment* and pushes button *Set*
 
 2. User enters tab *View Raw Data*

 3. User inputs scan number and list all the measuring points
 
 4. User views all the measurements

 5. User finds out the measurement with the strongest reflection and push button use

 6. GUI shifts to tab *Calculate UB* automatically

 7. User pushes button *Find Peak* with checking *Load HKL from file*

 8. GUI finds the peak center and load HKL

 9. User pushes button *Add peak* to add the peak to table

 10. User repeats step 2 to 9 to add other peaks

 11. User select the peaks that are linearly independent and pushes *Calcualte UB*

 12. GUI calculates UB matrix and show the result

 13. User may push *Index peak* to use the calculated UB matrix to index peaks in the table to check UB matrix.
 

Workflow to merge measurements in scan
++++++++++++++++++++++++++++++++++++++

Here is a typical use case to merge all the measuring points (Pt.) in a scan.

 1. User specifies *Experiment* and pushes button *Set*
 
 2. User enters tab *Merge Scan*

 3. User specifies the UB matrix either by *From tab Calculate UB* or by entering the values to text editor

 4. User pushes button *Set*.

 5. User specifies the frame in which the merged data will be in. If the target frame is Q-Sample-Sapce, then there is 
     no need to specify UB matrix;

 6. User specifies the scan numbers and push button *Add*;
     
 7. User specifies the base name for the output MDEventWorkspaces;

 8. User pushes button *Process*;

 9. User goes to MantidPlot to view the merged scan by SliceView or Vates.



Limitation
----------

- HFIR single crystal reduction GUI supports for instrument HB3A only from release 3.5.0 and nightly.
