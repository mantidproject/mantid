.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a WorkspaceGroup whose children are single-spectra workspaces
corresponding to focused banks, and saves their scattering angles
**two-theta** and **phi** (sometimes called **eta**) to a 4-column
format readable by MAUD.

The columns are, in order, **Bank number**, **Group number**,
**two-theta**, and **phi**. An example file looks something like this::

  bank :    1  group:     4    9.12160000026    0.0
  bank :    2  group:     5    8.15584000023    30.0000000009
  bank :    3  group:     6    8.03516799229    150.000000004
  bank :    4  group:     7    9.0611418429    180.000000005
  ... (and so on)


Usage
-----

.. testcode:: SaveBankScatteringAngles

   import os

   # Here we do a very rough-and-ready diffraction focusing
   # The workspace input_group has the same format as the result of a reduction from isis_pwoder
   input_ws = Load(Filename=FileFinder.getFullPath("HRP39180.raw"))
   focused_ws = DiffractionFocussing(InputWorkspace=input_ws,
                                     GroupingFileName=FileFinder.getFullPath("hrpd_new_072_01_corr.cal"),
				     OutputWorkspace="focused_ws")

   spectra = []
   for spec_num in range(3):
       spec = ExtractSingleSpectrum(InputWorkspace=focused_ws,
                                    WorkspaceIndex=spec_num,
				    OutputWorkspace="spectrum_{}".format(spec_num))
       spectra.append(spec)

   input_group = GroupWorkspaces(InputWorkspaces=spectra)

   output_file = os.path.join(config["defaultsave.directory"], "grouping.new")

   SaveBankScatteringAngles(InputWorkspace=input_group, Filename=output_file)

   with open(output_file) as f:
       contents = f.read().rstrip().split("\n")

   print("File contents:")
   for line in contents:
       print(line)

.. testcleanup:: SaveBankScatteringAngles

   os.remove(output_file)
			
Output:
       
.. testoutput:: SaveBankScatteringAngles

    File contents:
    bank :    0  group:     1    180.0000000000    0.0000000000
    bank :    1  group:     2    138.2823007994    180.0000000000
    bank :    2  group:     3    29.5674502659    0.0000000000
