.. _04_file_properties:

===============
File Properties
===============

Some algorithms will want to take a file/directory path as input. This
could be accomplished by using a string property. A
:ref:`mantid.api.FileProperty` has the advantage that if given just a
filename then it will search the current list of data directories for that
file rather than having to hard code paths in scripts.

A file property is declared as:

.. code-block:: python

    def PyInit(self):
        self.declareProperty(FileProperty(name="InputFile",defaultValue="",
                                          action=FileAction.Load,
                                          extensions = ["txt"]))

The ``extensions`` argument is optional. The ``action`` argument is one of
the following:

* ``FileAction.Load`` - The string is treated as a file that will be loaded
  during the algorithm. By default the property is not valid. The file is
  searched for on the paths specified by the data search directories.
* ``FileAction.OptionalLoad`` - As above, except a blank string is valid.
* ``FileAction.Save`` - The string is treated as a file that will be saved
  by the algorithm. If only a file name is given then a full path is produced
  by prefixing the name by the current default save directory.
* ``FileAction.OptionalSave`` - As above, except a blank string is valid.
* ``FileAction.Directory`` - The value of the string is treated as if it is a
  directory and must exist for the property to be valid.
* ``FileAction.OptionalDirectory`` - As above, except a blank string is valid.