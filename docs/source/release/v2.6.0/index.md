(v2.6.0)=

# Mantid 2.6.0 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_6_release/Fly.webp
---
class: screenshot
width: 300px
align: right
---
Volumetric plot of a fly head captured on ICON at the PSI.
```

## Release 2.6, Released: 2nd August 2013

This major release of Mantid has delivered many improvements to the instrument view and multi dimensional visualisation as well as a wide range of other improvements and new features.

**Add a note about the python API deprecation warning and algorithm deprecation** This release is the last release that v1 of the Python API will be supported. To ensure you can spot the scripts that might stop working in the next release Mantid will now show a warning in the log every time it is used. These warnings tell you the place that is using to old API and look like this:

```
Warning: Python API v1 call has been made  at line 3 in 'test_oldapi.py'[Rebin]
```

For information about how to migrate your script click {ref}`here <v2.6.0-framework>`.

Throughout the Mantid project we have fixed many of the bugs that have been reported to us. Thank you to everyone that has reported any issues to us. Please keep on reporting any problems you have, or crashes that occur to mantid-help@mantidproject.org or use this link.

## Installation Note

We have made a change to the location some of the user preferences are stored in Mantid. This means that the first time you start up Mantidplot it will not remember some of your settings such as window placements, and last entries on algorithm dialog boxes. This will however be automatically updated when you exit & rerun Mantidplot and it will restore your preferences.

## Key Features

### General

```{toctree}
---
titlesonly:
---
User Interface & Usability <ui>
Framework <framework>
```

### Scientific Techniques

```{toctree}
---
titlesonly:
---
Direct Inelastic <direct_inelastic>
Indirect Inelastic <indirect_inelastic>
SANS <sans>
Reflectometry <reflectometry>
Diffraction <diffraction>
```

### Full list

```{toctree}
---
titlesonly:
---
Full list of changes <full_list>
```

The same issues can be browsed in the [GitHub milestone](https://github.com/mantidproject/mantid/milestone/49?closed=1).
