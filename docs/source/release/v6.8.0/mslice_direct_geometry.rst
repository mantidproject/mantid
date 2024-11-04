MSlice
------

New features
############
- Upgraded Python from version 3.8 to 3.10.

BugFixes
########
- Stopped storing GDOS cuts in the ADS.
- Fixed a bug that caused an incorrect error propagation in GDOS intensity corrections.
- Fixed several bugs with script generation regarding cut plots.
- Fixed a bug that was causing underlying workspaces to be saved instead of the intended workspace.
- Fixed an bug that was causing an exception to be thrown when plotting cuts from the command line.
- Recoil and Bragg line colours are now propagated to generated scripts.
- Fixed a bug that was causing Windows file paths to be interpreted as unicode in generated scripts.
- Fixed a bug that was causing an exception when renaming workspaces containing special characters.
