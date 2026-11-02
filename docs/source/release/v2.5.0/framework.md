(v2.5.0-framework)=

# Framework

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Important News

```{figure} ../../images/2_5_release/PythonRetirementTimeline.webp
---
class: screenshot
width: 500px
align: right
---
Python Retirement Timeline.
```

### Retirement of Mantid Python v1

- We have been supporting two versions of the Python API since Mantid v2.0. Continuing to maintain both versions restricts further development and doubles the effort in maintaining the code.
- Therefore we will over this and the next 2 releases be phasing out v1 of the API “from mantidsimple”, in favor of it's replacement v2 “from mantid.simpleapi”.
  - This release of Mantid v2.5 will happily support both versions.
  - The next release v2.6 will run both versions, but use of v1 of the python API will create warnings.
  - Release 3.0 will only support v2 of the python API and will not run v1 scripts.
- How to Update your scripts to v2
  - Upload the scripts to the script repository before 2nd August. The development team will then migrate all scripts in the repository, they will contact the owner for testing and verification if necessary.
  - Try {ref}`migrate1to2.py <v2.1.0-python>` the automatic migration script.
  - Look at the python [changes notes](http://download.mantidproject.org/docs/current-release/python/html/changes.html) and try to update them yourselves.
  - Contact the Development Team for help.

## New Features

```{figure} ../../images/2_5_release/MERLINCalibration.webp
---
class: screenshot
width: 500px
align: right
---
Merlin with the centre door calibrated.
```

### Tube Calibration

- This release comes with calibration scripts for MERLIN, WISH and MAPS, support for further beamlines should be easy to include contact a member of the development team for help.
- The scripts can be found in \\scripts\\Calibration\\Examples under your installation directory.

### Neutronic Materials for Absorption corrections

- The SetSampleMaterial algorithm now allows you to set the neutronic properties of the material by providing the chemical formula.
- If you have first set the sameple material using SetSampleMaterial then you can leave the cross section and sample number density properties of the absorption correction algorithms blank, and the will be "read" from the material.

### Loading of other Data Sources

- LoadILL supports loading from IN5 and some other IN instruments at the ILL.
- LoadMcSTAS allow the result graphs from a McStas nexus file to be loaded into Mantid.

### Python

- CurveFitting functions can now be implemented in Python and slot into the optimization framework without care that they are Python functions. For more details see here
- Progress reporting has been exposed to Python algorithms. For an example see here and the *Squares.py* example shipped with Mantid in the `plugins/algorithms/Examples` directory.
- Control over the instrument view from Python has been greatly extended. The instrument view documentation page describes this in more detail.

## Under the hood

- We have updated many of the software components that help to build and form part of Mantid to more recent versions.
- A concerted push on compiler warnings and static analysis issues has improved code quality and should benefit the stability of the framework.
