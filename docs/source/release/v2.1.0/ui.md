(v2.1.0-ui)=

# User Interface & Usability

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_1_release/StartLiveData.webp
---
class: screenshot
width: 712px
align: right
alt: StartLiveData
---
```

## Live Data Processing

One of the major changes we have made in this release is a suite of functionality to allow Mantid to listen to and process live data as it is being recorded during and experiment. You can define simple to complex data processing workflows for the live data streams and any graphs you create from the resulting workspaces will update dynamically.

Mantid is a bit ahead of the curve here, and while Mantid can now handle this process the mechanisms at the SNS and ISIS to broadcast the data are not yet in place. They will be available over the next few months, but until then you can practice using a "FakeEventListener" that creates events at random between 40,000 and 60,000 ms TOF, and a FileEventListener that can replay event files to the system.

## Prototype Image Viewer

We have introduced a prototype image viewer into Mantidplot. Currently is is restricted to event workspaces, but will work histogram workspaces in the future. More details are in the {ref}`Single Crystal Diffraction <v2.1.0-single_crystal_diffraction>` release notes.

## Loading .py files

Previously you had to open the Python window to be able to load python script files. Now the File -> Load menu on the main screen will allow you to load them directly.

## Removing items from the interfaces menu

If there are interfaces you really aren't interested in seeing you can now remove them in the Manage Custom Menus.

```{figure} ../../images/2_1_release/ManageCustomMenus.webp
---
class: screenshot
width: 250px
align: right
alt: ManageCustomMenus
---
```

## Waterfall Plot with Logarithmic bins

The waterfall plot now displays logarithmically binned data correctly.

## Instrument view changes

There have been several changes to improve performance with large instruments in this release, in addition to the change in functionality below.

### Display of the neutronic or physical geometry

For instruments that have both a physical and neutronic representation you can now switch between displaying the two representations. The python code here shows an example:

```
 w1=LoadEmptyInstrument(r'C:/MantidInstall/instrument/VISION_Definition.xml')
 config['instrument.view.geometry'] = 'Physical'
 instrument_view = getInstrumentView('w1')
 instrument_view.show()
 config['instrument.view.geometry'] = 'Neutronic'
 instrument_view2 = getInstrumentView('w1')
 instrument_view2.show()
```
