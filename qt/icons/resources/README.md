This read me is to explain how to go about updating the current icon library
Currently in use is the material design logos by google: https://material.io/tools/icons/?style=baseline

The current version of material design that has been added has these icons: https://cdn.materialdesignicons.com/3.6.95/

If the file names differ from the source then the script may need to be re-evaluated.

- Delete the files from the folder containing this folder named `materialdesignicons.css`, `materialdesignicons-webfont-charmap.json`, and `materialdesignicons-webfont.ttf`
- Move the new .ttf file and .css files into the containing folder these can be gained from either the material design github or https://cdnjs.com/libraries/MaterialDesign-Webfont/
- Run the python script `updatematerialdesignjson.py` which will generate a new .json file
- Re-build the MantidQtIconsQt5 library target
- In theory now it should be updated, any icons that were removed or renamed will need to be replaced with new files.
