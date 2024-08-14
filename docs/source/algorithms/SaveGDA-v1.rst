.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a WorkspaceGroup whose children are single-spectra workspaces
corresponding to focused banks, and saves them to the MAUD-readable
``.gda`` format.

GDA Format
----------

GDA is a text-based format. The file is divided into sections for each
bank. Each section begins with a header, which has the following
format:::

  BANK <bank number> <number of points> <number of lines> RALF <min TOF> 96 <min TOF> <resolution> ALT

- ``bank number`` is simply and index for the bank
- ``number of points`` is the number of sets of data-points which will
  be saved from this bank
- ``number of lines`` is the number of lines the points will take
  up. Since there are 4 points per line, this will be ``number of
  points / 4``, rounded up
- ``min TOF`` is the minimum time-of-flight value in this bank
- ``resolution`` is the mean difference between adjacent TOF-values
  (normalised by TOF) in the bank

The data then follows. We have 4 points per line, where each point
consists of three values, ``TOF * 32`` (time-of-flight is scaled by 32
for legacy reasons in MAUD - see :ref:`save_gda_tof` for a detailed
explanation of where the TOF values come from), ``intensity * 1000``
and ``error * 1000``. ``TOF``, ``intensity`` and ``error`` correspond
to the ``x``, ``y`` and ``e`` columns of a Mantid workspaces
respectively.

All numbers apart from resolution are saved as integers. In addition,
every line, including the header, must be exactly 80 characters long,
so any short lines are right-padded with spaces.

.. _save_gda_tof:

D to TOF Conversion
-------------------

.. warning::

   TOF values in the output file will only match the actual recorded
   TOF values if the GSAS calibration file contains the correct
   conversion factors for each bank.

SaveGDA takes input in D-spacing, and applies the GSAS conversion
(explained at :ref:`EnggFitPeaks <algm-EnggFitPeaks>`), using
parameters from the calibration file, to convert back to
time-of-flight. The caveat here is that, if the calibration file
contains the wrong conversion factors, then the TOF values will not
match the ones that were actually recorded.

This is not necessarily a problem, as once the file is loaded into
MAUD, as long as the same conversion factors are used (ie the MAUD
calibration file should be created from the same GSAS calibration file
as was used to run SaveGDA), the data will still be aligned in
MAUD. When doing texture focusing with :ref:`ISIS_Powder for GEM
<isis-powder-diffraction-gem-ref>`, matching up all the calibration
files should be taken care of automatically.

This approach is taken because it was impractical to create a MAUD
calibration file containing the correct conversion factors for a
workspace with a large number of banks - we just don't have enough
data to do it. Instead we fake the time-of-flight recordings in order
to get good alignment in MAUD.

Usage
-----

.. testcode:: SaveGDA

   import os

   # Banks 1 to 4 of a previous texture focus in isis_powder
   # We don't use the full 160 banks as the test becomes too slow
   input_group = Load(Filename="GEM61785_D_texture_banks_1_to_4.nxs",
                      OutputWorkspace="SaveGDAtest_GEM61785")

   output_file = os.path.join(config["defaultsave.directory"], "GEM61785.gda")
   SaveGDA(InputWorkspace=input_group,
           OutputFilename=output_file,
	   GSASParamFile="GEM_PF1_PROFILE.IPF",
           # Assign spectra 1, 2 and 3 to bank 2 in calib file, and spectrum 4 to bank 3
	   GroupingScheme=[2, 2, 2, 3])

   with open(output_file) as f:
       file_contents = f.read().split("\n")

   # Print the header and the 4 lines from the middle of the file
   # rstrip the header just to make the doctest script happy
   print(file_contents[0].rstrip())
   for i in range(100, 104):
       print(file_contents[i])

.. testcleanup:: SaveGDA

   os.remove(output_file)
   mtd.remove("SaveGDAtest_GEM61785")

Output:

.. testoutput:: SaveGDA

    BANK 1 4246  1062 RALF  27388  96  27388 0.001 ALT
       40348    380   60   40388    285   52   40427    338   56   40467    218   47
       40507    232   49   40546    181   44   40586    171   43   40626    206   47
       40666    246   50   40706    161   40   40746    126   37   40786    124   37
       40826    131   40   40866    221   48   40906    157   40   40946    169   41

.. categories::

.. sourcelink::
