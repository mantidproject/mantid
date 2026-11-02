(v2.5.0)=

# Mantid 2.5.0 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_5_release/NonOrthogonalProjection.webp
---
class: screenshot
width: 415px
align: right
---
A non Orthogonal 3D projection of Spin Waves in Gd as measured on SEQUOIA
```

## Release 2.5, Released: 10th May 2013

This major release of Mantid has focussed on improvements to support single crystal diffraction and multi dimensional visualisation, this has not however prevented us delivering includes a wide range of improvements and new features. Please read the {ref}`User Interface <v2.5.0-ui>` and {ref}`Framework <v2.5.0-framework>` sections of the release notes and those of any techniques that interest you. In particular it is worth noting the {ref}`retirement plans for the old python API <v2.5.0-framework>` in the {ref}`Framework <v2.5.0-framework>` section.

Throughout the Mantid project we have fixed many of the bugs that have been reported to us. Thank you to everyone that has reported any issues to us. Please keep on reporting any problems you have, or crashes that occur to mantid-help@mantidproject.org or use this link.

## Key Features

### General

- {ref}`User Interface & Usability <v2.5.0-ui>`
- {ref}`Framework <v2.5.0-framework>`
  - {ref}`Python <v2.5.0-framework>`

```{toctree}
---
hidden:
---
ui
framework
```

### Scientific Techniques

```{toctree}
---
titlesonly:
---
Direct Inelastic <direct_inelastic>
Indirect Inelastic <indirect_inelastic>
SANS <sans>
Diffraction <diffraction>
Muon Analysis <muon>
```

### Visualization and Analysis Toolkit

```{toctree}
---
titlesonly:
---
Vates <vates>
```

#### Important Note

**In order to use the new visualisation tools described, you must update you version of ParaView. Please take installers for the 3.98.1 release of ParaView from [here](http://download.mantidproject.org/VatesDownload.psp)**. If you were previously using Paraview 3.10.1, and do not update to the new version of ParaView, the 3D visualisation tool additions will be disabled. For windows users, instructions on how to update Mantid to work with new ParaView are here.

### Full list

```{toctree}
---
titlesonly:
---
Full list of changes <full_list>
```

The same issues can be browsed in the [GitHub milestone](https://github.com/mantidproject/mantid/milestone/47?closed=1).
