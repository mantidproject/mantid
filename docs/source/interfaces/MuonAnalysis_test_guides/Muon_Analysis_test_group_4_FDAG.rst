.. _Muon_Analysis_TestGuide_4_FDAG-ref:

Muon Unscripted Testing: Group 4 (Frequency Domain Analysis)
=============================================================

.. contents:: Table of Contents
    :local:
    
Introduction
^^^^^^^^^^^^

These are unscripted tests for the :program:`Frequency Domain Analysis` interface.
In this case, runs from test group 2 are used, so those tests need to be run before these ones.

The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

Tests
^^^^^

Ensure that you have first run all the tests in group 2, then open the Frequency Domain Analysis GUI.

Test 1: Basic FFT
-----------------
- Set the workspace to "MUSR00062260;Group;bkwd;Asym;#1" 
- Click the calculate FFT button and a workspace should appear "FFTMuon" 
- Then plot spectrum 2 (there should be a total of 3 spectrums)
- You will see a few sharp peaks. 
- Untick the Imaginary Data and the a row should disappear
- Click the Calculate FFT button
- Look at the data table and there should be 6 rows.

Test 2: Advanced FFT
--------------------
- Plot spectrum 0 of the data
- Leave this plot open and make sure you can see it and the interface at the same time
- Change the "Apodization Function" to "Gaussian"
- Click the Calculate FFT button
- The plot should change, the data will be less noisy.
- Set the "Apodization Function" back to "None"
- Click the Calculate FFT Button
- Set the "Padding" to 0
- Click the Calculate FFT Button
- The peak should not be very clear

Test 3: MaxEnt
--------------
- Change the drop-down menu at the top of the interface to "MaxEnt"
- The interface should look different
- Click the Calculate MaxEnt button 
- This will be slow - a warning should be provided and the button disabled
- When it is complete a workspace should appear "FFTMuon" and the button should be enabled 

