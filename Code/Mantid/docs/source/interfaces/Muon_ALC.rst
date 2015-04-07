Muon ALC
========

.. contents:: Table of Contents
  :local:

Overview
--------

The Muon ALC interface, which is short for Avoided Level Crossing, aims at 
handling frequent analysis on e.g. HIFI. It uses simple point-and-click to 
analyse a sequence of datasets collected with different parameter values, for 
instance different magnetic fields, temperature, etc, and study how this 
affects asymmetry. There are currently three steps in the analysis. 

Data Loading
------------

The Data Loading step, provides an interface for the 
:ref:`PlotAsymmetryByLogValue <algm-PlotAsymmetryByLogValue>` algorithm, 
in which a sequence of runs are loaded through the fields 
*First* and *Last*. All datasets with run number between these limits will be 
loaded, and an error message will be shown if any of them is missing. The 
user must supply the log data that will be used as X parameter from the list 
of available log values.

.. interface:: ALC
  :widget: dataLoadingView
  :align: center
  :width: 800

Options
~~~~~~~

First
  First run of the sequence of datasets.

Last
  Last run of the sequence of datasets.

Log
  Log value to use as X parameter

Dead Time Correction
  Type of dead time corrections to apply. Options are *None*, in which case no 
  corrections will be applied, *From Data File*, to load corrections from 
  the input dataset itself, or *From Custom File*, to load corrections from a 
  specified nexus file.

Grouping
  Detector grouping to apply. *Auto* will load the grouping information contained 
  in the run file, while *Custom* allows to specify the list of spectra for both the 
  forward and backward groups.

Periods
  Period number to use as red data. The *Subtract* option, if checked, allows to 
  select the green period number that will be subtracted to the red data.

Calculation
  Type of calculation, *Integral* or *Differential*, together with the time limits.

?
  Shows this help page.

Load
  Computes the asymmetry according to selected options and displays it against the 
  chosen log value.

Baseline Modelling
------------------

In the Baseline Modelling step, the user can fit the baseline by selecting which 
sections of the data should be used in the fit, and what the baseline fit 
function should be. To select a baseline function, right-click on the *Function* 
region, then *Add function* and choose among the different possibilities. Then 
pick the desired fitting sections. 

.. interface:: ALC
  :widget: baselineModellingView
  :align: center
  :width: 400

Options
~~~~~~~

Function
  Right-click on the blank area to add a baseline function.

Sections
  Right-click on the blank area to add as many sections as needed to 
  select the ranges to fit.

?
  Shows this help page.

Fit
  Fits the data.
  
Peak Fitting
------------

In the Peak Fitting step, data with the baseline subtracted are shown in 
the right panel. The user can study the peaks of interest all with the same simple 
interface. To add a new peak, right-click on the Peaks region, then select 
*Add function* and choose among the different possibilities in the category Peak.

.. interface:: ALC
  :widget: peakFittingView
  :align: center
  :width: 600

Options
~~~~~~~

Peaks
  Right-click on the blank area to add a peak function.

?
  Shows this help page.

Fit
  Fits the data.

.. categories:: Interfaces Muon
