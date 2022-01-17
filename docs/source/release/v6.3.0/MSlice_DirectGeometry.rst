MSlice
------

BugFixes
########

- Fixed bug that prevented the print button from opening the print dialog
- Fixed bug that caused duplicated colour bars for slice plots and exceptions for cut plots when running a generated script while the original plot is still open
- Fixed bug in script generation for cut plots
- Cleaned up File menu for interactive cut plots
- Enabled editing for Bragg peaks on cut plots
- Prevented exception when generating script from plot created via script
- Added legends for recoil lines and Bragg peaks on slice plots
- Fixed bug that caused exceptions when scaling a workspace
- Added error message when attempting to load a file by path on the data loading tab
- Fixed bug that caused infinitely repeating energy unit conversions when changing the default energy unit
- When closing the dialog for adding a Bragg peak from a CIF file without selecting a CIF file the corresponding menu entry now remains unselected.