===============
Error Reporting
===============

Error Reporting
---------------

Error reporting has been enabled in place of the new last chance error handler. If mantid catches an unknown exception it will now display the dialog box below. Currently there is no automatic error reporting enabled if Mantid crashes to desktop but the same system is planned to be implemented in this case as well.

.. image::  ../../images/errorReporter.png
   :align: right
   :width: 800px

The three options do the following:

*Don't share any information*

The dialog box will close having sent no information. Mantid will either continue or terminate depending on which option has been selected at the top of the dialog.

*Share non-identifiable information*

An error report will be sent to errorreports.mantidproject.org. It will contain the following information:
 
- Operating System including version.
- Mantid version including git Sha1.
- System architecture.
- The date and time at which the crash occured.
- The mantid application you were using, currently this will always be mantidplot.
- The default facility you have set.
- The paraview version.
- The amount of time mantid was running prior to the crash.
- A hashed user id and a hashed host id.

*Yes, share information*

All the information from the non-identifiable information will be shared. In addition the optional name and email will be shared if given. 


