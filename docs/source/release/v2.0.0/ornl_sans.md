(v2.0.0-ornl_sans)=

# ORNL SANS

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## User Interface Improvements

Several improvements have been made to make the reduction workflow easier.

- A data catalog tab has been added. It lists all the users data sets in a given directory and provides basic information about each one. Users are also able to cut and paste the catalog into other applications for logging.

- A data stitching tab has been added. Users are able to select up to three I(Q) files or workspaces to stitch. The stitched data can then be saved to file.

## Minor Improvements and Fixes

- The long-standing problem with the TOF calculation for long wavelengths has been fixed.

- NaNs have been removed from the I(Qx,Qy) output.

- Pre-calculated sensitivity workspaces can be loaded directly by the reduction process. Until this improvement, the sensitivity had to be recalculated for each reduction process.

- Refactoring has also been done to improve the stability of the application.
