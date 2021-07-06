=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
############
- Fit functions `ElasticIsoRotDiff` and `InelasticIsoRotDiff` have been made available in the ConvFit tab in the Indirect Data Analysis
- A new option "autoconvolution" is added to the Abins Algorithm.
  This enables simulation of quantum orders up to order 10 by
  convolving the highest calculated spectrum (order 1 or 2) against
  the fundamentals spectrum repeatedly, before applying Debye-Waller
  terms. (NB: This has introduced small numerical differences from
  previous versions of Abins, because data is now binned before
  applying Debye-Waller terms. This difference will converge with
  small bin sizes.)

Improvements
############
- Abins: Thresholding of low-intensity modes has been changed. This
  impacts the second-order spectrum, especially at elevated
  temperature; excitations were being discarded on the basis of a low
  intensity in the fundamental spectrum, when they could contribute to a
  noticable peak in the second-order spectrum.

Bug Fixes
#########
- A bug has been fixed in Indirect data analysis on the F(Q)Fit tab, Multiple Input tab that allowed duplicate spectra to be added.


:ref:`Release 6.2.0 <v6.2.0>`

