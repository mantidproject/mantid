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
consists of three values, ``TOF * 32``, ``intensity * 1000`` and
``error * 1000``. ``TOF``, ``intensity`` and ``error`` correspond to
the ``x``, ``y`` and ``e`` columns of a Mantid workspaces
respectively.

All numbers apart from resolution are saved as integers. In addition,
every line, including the header, must be exactly 80 characters long,
so any short lines are right-padded with spaces.

Usage
-----

.. testcode:: SaveGDA

   import os

   input_ws = Load(Filename="ENGINX_277208_focused_bank_2.nxs")
   group = GroupWorkspaces(InputWorkspaces=[input_ws])

   output_file = os.path.join(config["defaultsave.directory"], "enginx_277208.gda")

   SaveGDA(InputWorkspace=group, Filename=output_file)

   with open(output_file) as f:
       file_contents = f.read().split("\n")

   # Print the header and the 4 lines from the middle of the file
   # rstrip the header just to make the doctest script happy
   print(file_contents[0].rstrip())
   for i in range(100, 104):
       print(file_contents[i])

.. testcleanup:: SaveGDA

   os.remove(output_file)

Output:

.. testoutput:: SaveGDA

   BANK 1 6277  1570 RALF  457400  96  457400 0.00024 ALT
     558443    111   15  558698     84   13  558953     98   14  559208     79   12
     559463     83   13  559718     68   11  559974     81   12  560229     99   14
     560484     78   12  560739    110   15  560994     57   11  561249     68   11
     561504     82   13  561760     71   12  562015     68   12  562270     81   13

