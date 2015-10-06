Simple command line tool to parse a set of ARIEL instrument definition files
and output the information in Mantid's XML instrument definition format.

USAGE: "ArielToMantidXML [path] [instrument]" where [path] is the path (relative
or absolute) to the set of ARIEL files and [instrument] is the name of the
instrument as given by the name of the 'root' ARIEL file.

Note that this will NOT produce a complete XML file. In particular, the detector
IDs (UDETs) will be missing and need to be entered manually. There is also currently
no 'shape' provided for the detectors, although there is at least some information
on this in the ARIEL files.