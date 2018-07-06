.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The POWGEN d-space map file format is a binary list of the conversion.
It needs to be a minimum size, determined by the PadDetID parameter.

Usage
-----

**Example - Save an OffsetsWorkspace to d-space map file:**

.. testcode:: ExSavePG3Dmap

  import os

  filepath = config["default.savedirectory"]
  if filepath == "":
    filepath = config["defaultsave.directory"]
  savefilename = os.path.join(filepath, "test_offset.dat")

  ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
  LoadCalFile(InputWorkspace=ws,CalFilename=r'PG3_golden.cal',MakeGroupingWorkspace='0',MakeMaskWorkspace='0',WorkspaceName='PG3_gold')
  SaveDspacemap(InputWorkspace="PG3_gold_offsets", DspacemapFile=savefilename)

  print("File created =  {} , file size =  {}".format(os.path.exists(savefilename),  os.path.getsize(savefilename)))

.. testcleanup:: ExSavePG3Dmap

  os.remove(savefilename)

Output:

.. testoutput:: ExSavePG3Dmap

  File created =  True , file size =  8168616

.. categories::

.. sourcelink::
