(v2.3.0-nxs)=

# nxs - NeXus file access

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

- This release ships with the [NeXus](http://www.nexusformat.org/) project's python package, which allows general access to a NeXus file from Python from within Mantid.

```{figure} ../../images/2_3_release/MantidPlotNxsPackage.webp
---
class: screenshot
width: 1100px
align: right
---
MantidPlot running a script using the nxs package
```

- The script in the image is given here for reference:

```
import nxs

filepath = FileFinder.getFullPath('SANS2D00002500.nxs') # Use Mantid to find file in datasearch path OR give a full path yourself

nxsfile = nxs.open(filepath,'r')
nxsfile.opengroup('raw_data_1') # ISIS raw data root group

print 'Entries under raw_data_1 group are:'
for entry in nxsfile.getentries():
	print 4*" " + entry
print

nxsfile.opendata("proton_charge")
proton_charge = nxsfile.getdata()
print "Proton charge entry = %f" % proton_charge
nxsfile.closedata() # Each call to opendata should be accompanied by a a closedata

nxsfile.closegroup() # Each call to opengroup should be accompanied by a a closegroup
nxsfile.close() # Equally each file opened should have a corresponding close
 from mantid.simpleapi import *
```
