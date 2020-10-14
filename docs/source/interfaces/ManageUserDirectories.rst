.. _ManageUserDirectories:

Manage User Directories
=======================

.. figure:: /images/MantidQtManageDirectories.jpg
   :alt: Manage User Directories Dialog
   :align: right
   :width: 455

This dialog allows you to adjust your settings (stored in the :ref:`Properties File`) for the data search directories which Mantid will search for data in, the option to also search the data archive, and the default save directory. It is accessible from the File menu, on the toolbar through it's icon |filefolder_icon| , through the "Help"->"First Time Setup" menu window, or from buttons on several custom user interfaces.

.. |filefolder_icon| image:: /images/MantidQtManageDirectoriesIcon.jpg

* In the text box next to the "Add Directory" button, you may type a path. Clicking on the button then adds this directory to the datasearch directories list. This is useful for adding things like network drives, for which browsing to is an impractical option.
* "Browse To Directory" brings up a file dialog in which you may select a directory to add to the list.
* "Remove Directory" will remove the selected directory from the list.
* "Move Up" will move the directory higher in the priority list.
* "Move Down" will move the directory lower in the priority list.
* "Search Data Archive" will tell Mantid which data archives it should try to search. See the description of ``datasearch.searcharchive`` in :ref:`Properties File` for information on the options.
* "Default Save Directory" is the location where Mantid will save most files. You may browse to find this by clicking on the "Browse to Directory" button.
* The "?" button will bring you to this help page.
* "Cancel" will exit the dialog **without** applying the changes you have made.
* "OK" will exit the dialog and apply the changes you have made to these settings.
