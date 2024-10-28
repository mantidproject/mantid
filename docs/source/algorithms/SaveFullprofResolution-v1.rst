.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A Fullprof's resolution file contains the peak profile parameters and
some powder diffractometer's geometry related parameters in a certain
format. This algorithm reads a TableWorkspace containing all the
information required by Fullprof's resolution file, and write out a text
file conforming to that resolution file's format.

Peak Profile Supported
----------------------

Here is the list of peak profile supported by this algorithm:

-  Back-to-back Exponential Convoluted with Pseudo-voigt peak profile
   (profile 9).
-  Thermal Neutron Back-to-back Exponential Convoluted with Pseudo-voigt
   peak profile (profile 10).

Instrument Profile Parameter TableWorkspace
-------------------------------------------

TableWorkspace as the input of this algorithm can be generated from
*CreateLeBailFitInput*, *RefinePowderInstrumentParameters* or
*LeBailFit*. To be noticed that the TableWorkspace output from
*RefinePowderInstrumentParameters* is usually an intermediate product.

Input TableWorkspace must have two columns, "Name" and "Value", as
column 0 and 1. There is no restriction on other columns.

For a multiple bank instrument, from the second column, the name of the
columns should be Value\_1, Value\_2 and so on. A row with parameter
name 'BANK' should be there to indicate the bank ID of a specific row of
parameters corresponding to.

How to use algorithm with other algorithms
------------------------------------------

Le Bail Fit
###########

This algorithm is designed to work with other algorithms to do Le Bail
fit. The introduction can be found in `Le Bail Fit <Le Bail Fit>`__.

Save For Multiple-Bank Resolution File
######################################

As SaveFullprofResolution can save 1 bank a time, in order to make a
multiple-bank .irf file, user should execute this algorithm a few times.
Except the first time, property 'Append' should be marked as 'True'.


Usage
-----

**Example - Save instrument profile parameters from a table workspace to Fullprof .irf file:**

.. testcode:: ExSaveIrf

  import os

  wsname = 'PG3Bank1Table'

  LoadFullprofResolution(Filename=r'PG3_Bank1.irf',OutputTableWorkspace=wsname)

  targetdir = config['default.savedirectory']
  if targetdir == "":
    targetdir = config['defaultsave.directory']
  savefile = os.path.join(targetdir, 'test.irf')

  SaveFullprofResolution(InputWorkspace=wsname, OutputFilename=savefile, Bank=1, ProfileFunction='Jason Hodge\'s function (profile 10)')

.. testcleanup:: ExSaveIrf

  import os
  os.remove(savefile)

.. categories::

.. sourcelink::
