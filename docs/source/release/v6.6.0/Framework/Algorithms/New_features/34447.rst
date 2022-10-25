- To allow files saved with :ref:`SaveAscii2` to be reloaded into Mantid and preserve rebinning behaviour,
  a ``Distribution=true`` flag is now written in the file header if the workspace is a Distribution.
  In this case, a column header is always written, even if the Save property ``ColumnHeader`` is set to false.
- To load an Ascii file, that was saved without a Distribution header flag, and set it as a Distribution,
  execute :ref:`LoadAscii2` with property ``ForceDistributionTrue=true``
