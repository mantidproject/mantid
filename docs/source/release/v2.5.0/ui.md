(v2.5.0-ui)=

# User Interface & Usability

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## New Features

```{figure} ../../images/2_5_release/InstViewNonDetectorComponents.webp
---
class: screenshot
width: 563px
align: right
---
The Instrument View can now display slots, guides, choppers and other components.
```

### Instrument View Changes

#### Displaying Guides, Choppers, slots etc

- The instrument view will now display any guides, choppers, slots and any other components defined in an instrument definition file.
- You can toggle the presence of these non detector components if they get in the way of the data by clicking on the "Display Detectors Only" option under the "Display Setting Button". These additional components are only visible in the 3D views, not the unwrapped 2D views.
- Along with the release the instrument definition file for MARI has been updated to include details of several of these additional instrument components. To add these to your instrument you can either use the MARI instrument definition as an example or contact the development team with details of the components you wish to add.

#### Masking and Grouping

```{figure} ../../images/2_5_release/MaskandGroup.webp
---
class: screenshot
width: 350px
align: right
---
Masking and Grouping tab
```

- The Masking and Grouping functionality have been merged onto a single tab and now share a common set of tools for drawing out regions to be masked or grouped. The option buttons at the top allow you to choose between masking and grouping
- The buttons to regions the masks have been simplified. You can either make changes to the view you see on the screen, or the underlying workspace data.
  - If you Apply a mask to the view you can revert it by clicking the clear All button. This will clear any masks on the view, and any regions that have not yet been applied. You can also save regions out as a region of interest or a mask to another workspace, a file or a cal file.
  - Once you are happy with the masked regions and want to apply it to the data click Apply to Workspace and the mask will be applied to the underlying data. Once the underlying data has been masked this cannot be cleared or reverted.

#### Pick Tab

- We have added a new tools that allows zooming within 2D unwrapped views of the instrument within the pick tab without having to move back to the render tab. This reduces the number of mouse clicks when using this view for manual peak picking.
- Peaks added to the workspace using the single crystal peak picking tool will now be automatically labelled with HKL values if either the workspace or peak list have a UB matrix defined.
- If you have overlaid a peak list and set the integration range in the instrument view, then only peaks whose centres fall within the integration range will appear.
- If the peak labels are obscuring the data, they can be turned off using the Peaks Options->Show Labels option on the Render Tab.

### LoadDAE has been replaced with StartLiveData

- The LoadDAE algorithm originally written for loading from the ISIS DAE, has now been deprecated and is now officially replaced with the StartLiveData algorithm. This supports loading from multiple facilities, and can process the data 'live' if required.
- The old LoadDAE algorithm remains available from the algorithm list for now, but is no longer supported and may be removed in the future.

### Help Improvements

- The searching facility on the mantid web pages have been improved, supporting multiple word searching and categorized results. It also now finds results from Python and C++ documentation in addition to the wiki pages.
- The Algorithm help pages have been improved to include usage examples and more images, The help information for many algorithms has been brought up to date and expanded.
- We have for the first time this release included offline help packaged within the Mantidplot installer. For this release it provides offline access to all of the Algorithm help pages, in future releases further help pages will be added to the help available offline.
  - If you wish to see the on-line help for an algorithm, for example to see the most recent details, then a link is provided at the top of each off line page.

```{figure} ../../images/2_5_release/ScriptRepository.webp
---
class: screenshot
width: 420px
align: right
---
The Script Repository.
```

### Script Repository

- The new script repository File->Script Repository is a tool to download/upload scripts to the mantid script repository as a way to share scripts with other Mantid users.
- When you first use the script repository it will ask you to specify where to install the repository. This can be any directory, including one where you already have some scripts.
- Any files you upload will be safely backed up and all versions of the script will be maintained, so if you want to revert to a previous version you just need to email mantid-help@mantidproject.org.
- At the moment the Repository supports downloading and uploading scripts from the repository, and will warn you if your changes conflict with changes on the repository. In the next release we plan on adding the ability to delete scripts you have uploaded and selects scripts to be auto updated when changes happen on the repository.
- This is one mechanism for updating your scripts to v2 of the Python API. Any scripts uploaded before the 30th of June will be updated by the development team. Other options are described on the {ref}`Framework Page <v2.5.0-framework>`.
