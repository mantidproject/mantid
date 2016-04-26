========================
Pearl Powder Diffraction
========================

Description
-----------
A wide variety of algorithms are available within Mantid for processing of data,
writing results files, etc.  These may be run interactively, one at a time, but
in general scripting (using the Python scripting language) enables a series of
algorithms to be called in a specific order to carry out more complex tasks, such
as data normalisation and writing data files suitable for input to Rietveld
refinement programmes (GSAS, FullProf, etc).

The script used to process Pearl data have been integrated to Mantid
which can be found inside the following directory on a Windows machine:
`C:\\MantidInstall\\scripts\\PearlPowderISIS`.

Run Requirement
---------------

A simple script is required to be ran inside the `Scripting Window
<http://docs.mantidproject.org/nightly/interfaces/ScriptingWindow.html>`_
on mantid in order to carry out the data normalisation.

The script which is written inside `Scripting Window <http://docs.
mantidproject.org/nightly/interfaces/ScriptingWindow.html>`_ on Mantid requires
the following from the user (followed up with an example usage):

- user name e.g: (`Bull`)
- cycle number e.g: (`15_4`)
- Attentuation file e.g: (`FileName.OUT`)
- optional: output directory - only if you wish to write out the files to preferred
  directory/location instead of the Pearl network. e.g: (`C:\\Mantid\\MantidOut\\`)
- run number/s e.g: (`92476_92479`)
- fmode e.g: (`trans`)
- ttmode e.g: (`TT70`)
