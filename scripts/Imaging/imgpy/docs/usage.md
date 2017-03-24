<!-- TOC -->

- [Pre-processing](#pre-processing)
    - [Loading data](#loading-data)
    - [Saving pre-processed images](#saving-pre-processed-images)
    - [Selecting filters](#selecting-filters)
    - [Running _only_ pre-processing](#running-_only_-pre-processing)
    - [Saving pre-processed images](#saving-pre-processed-images-1)
- [Reconstruction](#reconstruction)
    - [Preparing center of rotation](#preparing-center-of-rotation)
        - [Usage of --imopr ... cor](#usage-of---imopr--cor)
        - [Usage of --imopr ... corwrite](#usage-of---imopr--corwrite)
    - [Specifying centers of rotation for reconstruction](#specifying-centers-of-rotation-for-reconstruction)
    - [Running _only_ a reconstruction](#running-_only_-a-reconstruction)
    - [Running a reconstruction](#running-a-reconstruction)
- [Post-processing](#post-processing)
    - [Running _only_ post-processing](#running-_only_-post-processing)
    - [Selecting filters](#selecting-filters-1)

<!-- /TOC -->

# Pre-processing
## Loading data
Sample image data can be loaded with the `-i` or `--input-path` command.

Dark image data can be loaded with the `-D` or `--input-path-dark` command.

Flat image data can be loaded with the `-F` or `--input-path-flat` command.

The image files must be in separate folders for each (sample, dark, flat).
All the Dark and Flat image files will be loaded, and then averaged before applying any normalisations.

Both Flat and Dark images must be provided, or both must be absent. A pre-processing run cannot be ran with only one.

Examples:

`python main.py -i ~/some/folders/sample`

`python main.py -i ~/some/folders/sample -D ~/some/folders/dark -F ~/some/folders/flat`

---

## Saving pre-processed images
In order for any output to be produced, an output folder needs to be specified using `-o` or `--output-path`. Otherwise _no images will be saved_.

`python main.py -i ~/some/folders/sample -D ~/some/folders/dark -F ~/some/folders/flat -o /tmp/imgpy/`

With the current command line so far, the pre-processing images _will not be saved_. To save out the pre-processing images the user has to specify explicitly that they want the images to be saved, and an output directory **must** be provided.

To save out the pre-processed images, the flag `-s` or `--save-preproc` needs to be added.
The subdirectory can be specified using the `-p` or `--preproc-subdir` flags, and it will be created inside the output folder. This allows to save out images with different pre-processing parameters without flooding the folder space.

`python main.py -i ~/some/folders/sample -D ~/some/folders/dark -F ~/some/folders/flat -o /tmp/imgpy/ --rotate 1 --pre-median-size 3 --pre-outliers 8 --pre-outliers-radius 8 --only-preproc --save-preproc`

## Selecting filters
The available filters can be found in the filters.md document. To add a filter to the pre-processing just add it to the command line and it will be executed:

`python main.py -i ~/some/folders/sample -D ~/some/folders/dark -F ~/some/folders/flat -o /tmp/imgpy/ --rotate 1 --pre-median-size 3 --pre-outliers 8 --pre-outliers-radius 8`

## Running _only_ pre-processing
If you want to do _only_ the pre-processing and then **exit** you can do so by adding -s or --only-preproc to the command line.

`python main.py -i ~/some/folders/sample -D ~/some/folders/dark -F ~/some/folders/flat -o /tmp/imgpy/ --rotate 1 --pre-median-size 3 --pre-outliers 8 --pre-outliers-radius 8 --only-preproc`

# Reconstruction
## Preparing center of rotation
### Usage of --imopr ... cor
Initial guesses for COR should be done via `--imopr <slice_id> cor` functionality.

`python main.py -i ~/some/folders/pre_processed -o /tmp/imgpy/ --imopr 340 cor`
This script will run the standard tomopy find_center on the 340th slice. This should only be used as an initial COR guess.
### Usage of --imopr ... corwrite
The accurate calculation of the COR requires finding the COR on a few different slices. Preferably from different areas of the sample, e.g. on 6 slices, equidistant from each other.

This can be done using the `--imopr <slice_id> <cor_start> <cor_end> <cor_step> corwrite` functionality. 

This will reconstruct slice id with the CORs in range `[cor_start, cor_end)`, with a step of `cor_step`. This allows to see the difference each COR makes to the reconstructed slice.

This function **requires** an output path!

`python main.py -i ~/some/folders/pre_processed -o /tmp/imgpy/ --imopr 340 140 160 1 corwrite`
This script will reconstruct slice 340 with CORs from [140, 160) with step 1, and write the reconstructed images with each COR in the provided output directory.

## Specifying centers of rotation for reconstruction
After we have around 6 slices and their respective CORs, we can run the reconstruction with this script:

`python main.py -i ~/some/folders/pre_processed -o /tmp/recon/ --cor-slices 0 125 250 375 500 --cors 340.3 341.4 341.5 342.6 342.7 --reuse-preproc`

The `--reuse-preproc` allows us to skip the pre-processing step and just run the reconstruction.

The `--cor-slices 0 125 250 375 500 --cors 340.3 341.4 341.5 342.6 342.7` specifies the following:
- Slice 0 has COR 340.3
- Slice 125 has COR 341.4
- Slice 250 has COR 341.5
- Slice 375 has COR 342.6
- Slice 500 has COR 342.7

The rest of the CORs for the sample will be interpolated using this data. If the COR is not accurate enough, provide more CORs around the are that is inaccurate. This method has not been extensively tested, but it has so far produced good results that serve as a tilt correction.

<!-- ^ add -o -->
# Post-processing
## Running _only_ post-processing
`python main.py -i ~/some/folders/reconstructed -o /tmp/post_proc/ --only-postproc`

The `--only-postproc` flag allows to load in reconstructed data and just apply available post-processing filters to the data.