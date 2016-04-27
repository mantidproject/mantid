===============================
Pearl Powder Diffraction Script
===============================

Overview
--------

A wide variety of algorithms are available within Mantid for
processing of data, writing results files, etc.  These may be run
interactively, one at a time, but in general scripting (using the
Python scripting language) enables a series of algorithms to be called
in a specific order to carry out more complex tasks, such as data
normalisation and writing data files suitable for input to Rietveld
refinement programmes (GSAS, FullProf, etc).
The script used to process Pearl data have been integrated to Mantid
which can be found inside the following directory on a Windows machine:
`C:\\MantidInstall\\scripts\\PearlPowderISIS`.

Further extensions can be expected for next releases as it is under
active development and feedback is very much welcome. The sections
below describes how the script can be ran and help user develop
understanding of the script. By utilising this script it is assumed
that the scientists/users at ISIS have permission to read and write
from the network. Where `P:\` drive is linked to `\\britannic\Pearl\`
and `X:` drive is linked to `\\isis\inst$\NDXPEARL\Instrument`.

Run Requirement
---------------

A simple script is required to be ran inside the `Scripting Window
<http://docs.mantidproject.org/nightly/interfaces/ScriptingWindow.html>`_
on mantid in order to carry out the data normalisation.

The script which is written inside `Scripting Window <http://docs.
mantidproject.org/nightly/interfaces/ScriptingWindow.html>`_ on Mantid
requires the following from the user (followed up with an example
usage):

- user name e.g: (`Bull`)
- cycle number e.g: (`15_4`)
- run number/s e.g: (`92476_92479`)
- fmode e.g: (`trans`)
- ttmode e.g: (`TT70`)

- optional: Attentuation file directory - if you would wish to use
  a different Attentuation file e.g:
  (`P:\\Mantid\\Attentuation\\PRL112_DC25_10MM_FF.OUT`)
- optional: Output directory - only if you wish to write out the
  files to preferred directory/location instead of the Pearl network.
  e.g: (`C:\\Mantid\\MantidOut\\`)

To run the script, an example can be found within the usage section.

File & Folders
--------------

You are not require to set calibration and raw files directory,
unless you are utilising calibration files from your local machine.
The script will automatically find the correct calibration files
on the Pearl network according to the input run number, fmode and
ttmode. All the calibration files can be found in
`P:\\Mantid\\Calibration`, whereas all the Attenuation (`.OUT`)
files can be found within the following directory
`P:\\Mantid\\Attentuation`.

The output files by default will be written out to the following
location: `P:\\users\\cycle number\\user name` e.g:
`P:\\users\\Cycle_15_4\\Bull\\`, however a personalised output
directory can be provided instead like explained in Run
Requirement section.

Usage
-----

**Example - General Script Utilised To Process Powder Diffraction With Pearl**

.. code-block:: python

   import pearl_routines

   # set up the user name and cycle number
   pearl_routines.PEARL_startup("Bull","15_4")

   # set up the directory to the attentuation file
   pearl_routines.PEARL_setattenfile(new_atten="P:\\Mantid\\Attentuation\\PRL112_DC25_10MM_FF.OUT")

   # set up the run number/s, fmode, ttmode, use attentuation file (True/False), do vanadium normalisation (True/False)
   pearl_routines.PEARL_focus("92476_92479",fmode="trans",ttmode="TT70",atten=True,van_norm=True)


