(v2.4.0-framework)=

# Framework

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## New Features

### File Loading Improvements

We have implemented a whole collection of improvements to file loading and archive searching in this release:

- The loading of "raw" nexus files, that is ones straight from the instrument has been improved such that if the file contains a full instrument definition then this will be used in preference to any in the instruments directory. This is important to allow calibrated instrument definitions to be stored within the same file as the "raw" data from the instrument.
- Mantid can now load weights associated with events in "raw" event nexus files, this is useful for loading simulated data.
- We have corrected the validation in the load dialog to remove a problem where Mantid would report a validation error on the workspace name incorrectly.
- We have reduced the time Mantid takes to give up if it cannot find the file if it cannot find it in the archive. It is not perfect yet, but it is much better.
- A bug that prevented POWGEN files from being found by the file finder has been resolved.
- An issue that could freeze mantid when loading an ISIS raw file if there were no associated log files has been fixed.
- Ubuntu and RHEL6 we have corrected an issue where the extension filtering of files was not working correctly when browsing the file system.
- A bug that occurred when loading multiple HDF4 files has been resolved, this was previously found with older muon files.
- Mantid can now successfully load older Nexus files from the SNS.
- Changes to Instrument definition files are now automatically detected using the file modification time. It is no longer necessary to change the modification date in the header to ensure the updates are used for newly loaded files.

```{figure} ../../images/2_4_release/BackToBackExponentialWithConstBackground.webp
---
class: screenshot
width: 200px
align: right
alt: BackToBackExponentialWithConstBackground
---
```

#### Optimisation & curve fitting improvements

##### Stability of back to back exponential curve fitting

- The Back to back exponential algorithm has been extensively rewritten to improve the stability and reliability of curve fitting using this function.

##### Option to output separate spectra for each function within a CompositeFunction on Fit

- The {ref}`Fit <algm-Fit>` algorithm now takes an extra argument, `OutputCompositeMembers`. If the main function is a CompositeFunction and this argugment is set to `True` the output will contain an additional spectrum for each function contained within the main composite function.

```{figure} ../../images/2_4_release/CompositeFunctionPlot.webp
---
class: screenshot
width: 850px
align: right
---
A composite function containing a Back2BackExponential with a LinearBackground. The output shot shows both the separate functions plus the sum.
```

#### Visualization of Structure Factors from Molecular Dynamics Simulations

- The ability to visualize the intermediate structure factors I(Q,t) derived from molecular dynamics simulation trajectories has been implemented via the new {ref}`LoadSassena <algm-LoadSassena>` algorithm. [Sassena](http://www.sassena.org/) is a highly scalable parallelized software for calculating neutron and xray scattering intensities from all-atomic molecular dynamics simulations. The user may load the Sassena ouput (an HDF5 file) resulting in a groupworkspace. Each workspace member holds a different structure factor.The algorithm {ref}`SassenaFFT <algm-SassenaFFT>` performs the fourier transform using the groupworkspace as input. The user can enter the simulation temperature to apply the detail balance condition (see the {ref}`SassenaFFT <algm-SassenaFFT>` wiki page for details).

Here’s a snapshot of a Mantid Session showing the intermediate and final structure factors after using {ref}`LoadSassena <algm-LoadSassena>` and {ref}`SassenaFFT <algm-SassenaFFT>` on a Sassena output file. The simulated system corresponds to a [NAMD](http://www.ks.uiuc.edu/Research/namd) simulation of a concentrated solution of Li+Cl- in water at room temperature. [Pymol](http://www.pymol.org/) was used to render a snapshot of the system.

```{figure} ../../images/2_4_release/SasssenaSession.2.4.jpg
---
class: screenshot
width: 650px
align: right
---
Intermediate and final structure factors from a simulation of a concentrated solution of Li+Cl- in water at room temperature.
```

#### Instrument Definition File Improvements

- We had added a locations tag that can hugely simply the job of creating lots of components (pixels, tubes, anything) along either a linear or radial direction. The example below shows how this one \<locations> tag allowed us to shrink the LET definition file by 65%. More information can be found {doc}`here </concepts/InstrumentDefinitionFile>`

```
<type name="LETtube" outline="yes">
<component type="pixel">
  <locations y="-2.00304" y-end="2.00304" n-elements="1024" />
</component>
</type>
```

- We had two tags, "mark-as" and "is" that referenced to very similar concepts. We have standardised on "is" and deprecated the use of "mark-as". Some historic saved nexus files will contain "mark-as" tags and will emit warnings to the log, but will continue to work fine.
- Mantid can now support multiple parameter files as well as multiple instrument definition files. In order of preference Mantid will load the first of these files:
  1. The Parameter file with a matching name to the instrument definition file, that is in the same directory as the instrument definition file. e.g.MERLIN_Definition_bigger_doors.xml would load MERLIN_Parameters_bigger_doors.xml
  1. The Parameter file with a matching name to the instrument definition file, but with the parameter file in the instrument directory.
  1. The generically named parameter file in the instrument directory. e.g.MERLIN_Definition_bigger_doors.xml would load MERLIN_Parameters.xml
- The errors reported if an id list is missing has been improved.
- Within a parameter file if you want to refer to a component of the instrument by the full path name of the component you now can. e.g. WISH/Bank1/Tube4/Det45.

#### Instrument Updates

- POLARIS
- HYSPEC
- TOPAZ

#### Algorithms

- {ref}`AverageLogData <algm-AverageLogData>` returns the proton charge averaged value of a sample log.
- {ref}`CloneWorkspace <algm-CloneWorkspace>` now also works on TableWorkspaces
- {ref}`DetermineChunking <algm-DetermineChunking>` now supports a wider variety of file types.
- {ref}`FilterByLogValue <algm-FilterByLogValue>` now supports integer based log values.
- {ref}`FilterByXValue <algm-FilterByXValue>` Filters the events in an event workspace according to a minimum and/or maximum value of X.
- {ref}`FilterEvents <algm-FilterEvents>` is the erly implementation of a more powerfull and flexible filtering framework for event workspaces.
- {ref}`LoadSassena <algm-LoadSassena>` and {ref}`SassenaFFT <algm-SassenaFFT>` enable the loading and the Fourier Transform of intermediate structure factors computed by Sassena from molecular dynamics simulation trajectories.
- {ref}`ResampleX <algm-ResampleX>` algorithm which allows for specifying the number of bins, and calculates the binning parameters for rebinning.
- {ref}`SaveAscii <algm-SaveAscii>` now has the option to exclude the header information.
- {ref}`SaveIsawQvector <algm-SaveIsawQvector>` Saves a file of Qx,Qy,Qz for each event within an event workspace.
- {ref}`SaveMask <algm-SaveMask>` will saving the masking of a workspace to a file.
- {ref}`SaveNexus <algm-SaveNexus>` the progress reporting has been improved.
- {ref}`ScaleX <algm-ScaleX>` now supports negative factors.
- {ref}`SortPeaksWorkspace <algm-SortPeaksWorkspace>` can sort the peaks workspace by a single selected column.
