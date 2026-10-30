(v3.3.0)=

# Mantid 3.3.0 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/3_3_release/MantidSplashScreen.png
---
class: screenshot
width: 385px
align: right
---
Our thanks to Renee Manning in the SNS communications team for our new artwork.
```

## Release 3.3, Due for Release: 16th January 2015

This release sees the inclusion new normalization techniques both in MD workspaces and 1D graphs, together with new interfaces for fitting multiple datasets and visually filtering event data. Behind the scenes a new Instrument repository allows new and updated instrument definitions to be deployed to everyone without having to install a new version of Mantid. We also want to make sure we focus our developments on the areas and algorithms within Mantid that are actually used, therefore we have started to collect some simple anonymous usage data to ensure we focus our efforts where we can make the most impact, and perhaps in the future deprecate and remove algorithms that are not actually used. This release only reports the version of Mantid and the operating system it is used on, but future releases will begin to track algorithm and feature usage. We do take your privacy seriously and all of this will be anonymous and is not possible to refer back to specific people or machines, experimental data or results will never be recorded.

These are just a few headline points for the changes in this release, so please take a look at the release notes which are filled with important changes and improvements in many areas. The development team has put a great effort into making all of these improvements within Mantid, and we would like to thank all of our beta testers for their time and effort helping us to make this another reliable version of Mantid.

Throughout the Mantid project we put a lot of effort into ensuring Mantid is a robust and reliable product. Thank you to everyone that has reported any issues to us. Please keep on reporting any problems you have, or crashes that occur to mantid-help@mantidproject.org or use our contact page.

Installation packages can be found on our [download page](http://download.mantidproject.org/) which now links to sourceforge to mirror our download files around the world, you can also access the source code on [GitHub](https://github.com/mantidproject/mantid/releases/tag/v3.3.0).

### Citing this release

Mantid (2015): Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project. \
DOI: [10.5286/SOFTWARE/MANTID3.3](https://doi.org/10.5286/SOFTWARE/MANTID3.3) (with full author list)

## Changes

```{figure} ../../images/3_3_release/1D_Graph_normalization.png
---
class: screenshot
width: 700px
align: right
---
The default plotting for graphs has changed to plot data normalized by bin with for Histogram data.
```

```{toctree}
---
titlesonly:
---
Diffraction <diffraction>
```

The detailed release notes for the other areas (User Interface & Usability, Documentation, Framework, Python, ISIS Auto Reduction, Direct Inelastic, Indirect Inelastic, SANS, Muon Analysis, Reflectometry) were on separate wiki pages that were not preserved when the Mantid wiki was retired.

## Full Change Listings

For a full list of all issues addressed during this release please see the [GitHub milestone](https://github.com/mantidproject/mantid/milestone/1?closed=1).

The [ISIS Release Presentation](https://github.com/mantidproject/documents/blob/master/Presentations/Release%20Presentations/3.3/Release%203.3%20Presentation.pdf?raw=true) is also available.
