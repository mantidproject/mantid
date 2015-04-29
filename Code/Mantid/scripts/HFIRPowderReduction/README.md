Tasks
-----
  
  6. Find out why the vanadium runs (379-10/11) look funny, i.e., oscilating curves;
  7. Talk with Clarina how to deal with vanadium spectrum with peaks striped; 
  8. Tab *Normalized*: **Normalization Monitor** should have a default value as the average of monitor counts
  9. Check whether a scan has been loaded as **Load** is pushed; 
  10. Enable *SaveGSS*() work with reactor-source powder diffractometers;


Finished Tasks
--------------
  
  1. Make tab *Raw Detector* work;
  2. Make tab *Individual Detector* work;
  3. Implement tab *vanadium*;
  4. (75%) Each tab with *Previous* and *Next* buttons should have a data structure to record whether a certain plot is already on canvas
  5. Tab *Normalized*: **Normalization Monitor** is not parsed;
  6. Tab *Normalized*: apply reduction record to binning push buttons and avoid plotting one data twice on same plot with *Prev Scan* and *Next Scan*;
  7. Tab *Normalized*: **Save** does not work well with option *gsas and fullprof*;
  8. Tab *vanadium*: implement **Smooth Data**;
  9. Tab *vanadium*: implement **Save**;
  10. Tab *Multiple Scans*: implement **Save Merged**
  11. Tab *Multiple Scans*: implement **Save All**


Use cases for tabs
------------------

  1. **Raw Detectors**: Visualize the reading of detectors directly coming out of the raw data
    * Plot N lines for N Pts.;
    * Highlight (make it thicker) the Pt that is interested;
    * New from Mantid:  *ReadRawSpiceSignal(Pts)*;
  2. **Individual Detector**: Visual the readings of one detector across an experiment
    * Plot the counts of any individual detector;
    * Able to change the X-axis from 2theta to arbitrary sample environment log;
    * New from Mantid: *ReadRawSpiceSignal(DetectorID, XLabel)*;
  3. **Normalized**: Reduce one scan each time
    * Plot the reduced data
    * Automatically locate detector efficiency file
    * New from Mantid: *ConvertCWPDMDToSpectra(ExcludedDetectors=[])*
    * New from Mantid: *ConvertSpiceDataToRealSpace(DetectorEfficiencyTable)*
  4. **Multiple Scans**: Reduce a set of scans
    * Reduce a set of scans and plot in 2D/water-fall mode;
    * Able to merge all the scans;
    * New from Mantid: *ConvertCWPDMDToSpectra(ExcludedDetectors=[])*
  5. **Vanadium**: strip vanadium peaks
    * Strip vanadium peak with unit 'Degrees' because of the binning (range and step size) must be respected;
    * Peaks' position should be calculated and indicated auotmatically;
    * *Mantid::StripPeaks()* will be called instread of *StripVadadiumPeaks()* because
      the later one only works in d-spacing;
  6. **Advanced Setup**
    * URL for raw data files; 


Suggested workflow for *Normalization*
======================================

Here is a typical use case for reduce data via tab *Noramlization*

 1. User specify *Exp No* and *Scan No*;
 2. HFIR-PDR-GUI loads SPICE data according to experiment number and scan number;
 3. HFIR-PDR-GUI checks whether vanadium correction file, i.e., detector efficiency file exists on server;
 4. HFIR-PDR-GUI checks whether excluded detectors file exists on server;
 5. HFIR-PDR-GUI checks log **m1** for wavelength and set to *Wavelength* ;
 6. User may specify detector efficient file;
 7. User specifies *Bin Size*; (Or auto bin size determination???)
 8. User pushes button *Load Data*;
 9. HFIR-PDF-GUI reduce data in unit of *2theta* by taking accounting of 
   * Detector efficiency;
   * Excluded detectors; 
 10. HFIR-PDR-GUI plots the reduced data;
 11. User may rebin by different binning parameters or unit;
 12. User may save the result;


Suggested workflow for *Multiple Scans*
=======================================

It might be confusing to use the functionalities in tab *Multiple Scans*. 
Here is the suggested workflow to reduce multiple scans and possibly merge them.

 1. Set up *Exp No* and range of scan numbers;
 2. Push button *Load All* to load and reduce all runs specified in previous step to single-spectrum diffraction pattern;
 3. Waterfall plot all reduced scans in default;
 4. Optinally plot all data in 2D fill plot;
 5. User can delete some scans from the reduced scans via GUI or input text edit;
 6. Push button *Merge* to merge the scans;
 7. Push button *Save All* to save all individual scans to files;
 8. Push button *Save Merged* to save the merged scans to one file; 


Features (Implemented)
----------------------

 * Automatic wavelength mapping (in progress);


HB2A Data Reduction
-------------------

Raw experimental data are to be corrected by (1) detector efficiency, (2) vanadium spectrum and etc. 
Experiments are done with neutrons with various wavelengthes.  
There information can be retrieved from HB2A's data repository accessible from internet. 

Experiment setup and sample log
===============================

 1. **Wavelength**: There are three settings for neutron wavelength, referenced by sample log *m1*. 
   * Ge 113: :math:`\lambda = 2.41 \AA`, m1 = 9.45  (The **error** can be 0.05, such that in Exp 231 scan0001, m1=9.5)
   * Ge 115: :math:`\lambda = 1.54 \AA`, m1 = 0
   * Ge 117  :math:`\lambda = 1.12 \AA`, No used

 2. **Collimator translation**: There are two status for collimator, which is specified by sample log *colltrans*
   * *IN*:  colltrans = 0
   * *OUT*: colltrans = +/-80


Raw data correction files
=========================

 1. **Detector efficiency**: 
   * File name: *HB2A_exp0IJK__GE_abc_XY_vcorr.txt* where
    - IJK is the experiment number
    - abc is the GE set up.  It can be 113, 115 or 117
    - XY is either IN or OUT. 
    - Exmaple: *HB2A_exp0400__Ge_113_IN_vcorr.txt*
   * Web address: *http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0IJK__Ge_abc_IN_vcorr.txt*
    - IJK is the experiment number
    - abc is the GE set up.  It can be 113, 115 or 117
    - XY is either IN or OUT. 
    - Exmaple: *http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400__Ge_113_IN_vcorr.txt*

 2. **Excluded detectors**:  Some detectors might be exluded from the experiment for some reason.  It is recorded in some excluded detectors' file.
   * File name: *HB2A_exp0IJK__exclude_detectors.txt*
    - IJK is the epxeriment number
    - Exmaple: *HB2A_exp0400__exclude_detectors.txt*
   * Web address: *http://neutron.ornl.gov/user_data/hb2a/expIJK/Datafiles/HB2A_exp0IJK__exclude_detectors.txt*
    - IJK is the experiment number
    - Example: *http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400__exclude_detectors.txt*

 3. Detector gaps: The 2-theta gap (in unit degrees) can be changed among cycles. 
   * Location example: *http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400__gaps.txt*





