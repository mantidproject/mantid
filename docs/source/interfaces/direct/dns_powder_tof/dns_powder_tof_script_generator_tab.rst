.. _dns_powder_tof_script_generator_tab-ref:

DNS Script Generator Tab
------------------------

.. image::  ../../../images/DNS_interface_powder_tof_script_generator_tab.png
   :align: center
   :height: 400px

\

This tab enables a user to generate and run a mantid script for data reduction
by clicking on the **Generate Script** button.

**Automatic Script Filename** checkbox is selected by default and the output
script filename in this case will be set automatically to
``script_{sample name}_from_{first file number}_to_{last file number}.py``. Alternatively,
a user can deselect this box and set the output script filename manually. The
script is saved to the **Script Directory** selected in the
:ref:`Paths <dns_powder_tof_paths_tab-ref>` tab. The path is shown in the status
line.

Running of the script can be aborted, by clicking the **Abort Loading** button,
but then the data will not be displayed under the
:ref:`Plotting <dns_powder_tof_plotting_tab-ref>` tab.
The displayed script window cannot be edited inside the GUI, but can be copied to the
clipboard using the **Copy Script to Clipboard** button and reducted elsewhere.
