# Contents

<!-- TOC -->

- [Contents](#contents)
- [Utility](#utility)
  - [Merge pre-processing images into a stack. This will not apply any filters as pre-processing, it will just pack all of the images into a stack](#merge-pre-processing-images-into-a-stack-this-will-not-apply-any-filters-as-pre-processing-it-will-just-pack-all-of-the-images-into-a-stack)
- [Find Center Runs](#find-center-runs)
  - [Single image and no crop](#single-image-and-no-crop)
  - [Full `RB000888 test stack larmor summed 201510`, spheres crop](#full-rb000888-test-stack-larmor-summed-201510-spheres-crop)
- [Run Reconstruction](#run-reconstruction)
  - [Single Image <br>](#single-image-br)
  - [Full `RB000888 test stack larmor summed 201510`, bolts crop, good air region](#full-rb000888-test-stack-larmor-summed-201510-bolts-crop-good-air-region)
  - [Full `RB000888 test stack larmor summed 201510`, spheres crop, good air region](#full-rb000888-test-stack-larmor-summed-201510-spheres-crop-good-air-region)
  - [Full `RB000888_test_stack_larmor_summed_201510` dataset, bolts crop, **BAD** air region](#full-rb000888_test_stack_larmor_summed_201510-dataset-bolts-crop-bad-air-region)
- [ImageJ `GetSelectionCoordinates` Macro](#imagej-getselectioncoordinates-macro)
- [Python local tests](#python-local-tests)
  - [Pyfits load image stack](#pyfits-load-image-stack)
  - [Test loading single images and image stack](#test-loading-single-images-and-image-stack)
- [Astra Reconstructions](#astra-reconstructions)

<!-- /TOC -->
 

# Utility
## Merge pre-processing images into a stack. This will not apply any filters as pre-processing, it will just pack all of the images into a stack

```python
python tomo_main.py  
-i=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full  
-o=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000  
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py -i=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full -o=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --data-as-stack


<br/>

# Find Center Runs

## Single image and no crop

```python
python tomo_main.py  
--num-iter=5  
--input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_stack_larmor_summed  
--input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed  
--input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed  
--region-of-interest='[0.000000, 0.000000, 511.000000, 511.000000]'  
--rotation=1  
--max-angle=360.000000  
--find-cor  
--output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000  
--tool=tomopy 
```

For Copy/Paste to terminal:
>python tomo_main.py --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_stack_larmor_summed --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[0.000000, 0.000000, 511.000000, 511.000000]' --rotation=1 --max-angle=360.000000 --find-cor --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --tool=tomopy 

EXPECTED RESULTS:
> COR: 265.0

---

## Full `RB000888 test stack larmor summed 201510`, spheres crop

```python
python tomo_main.py  
-n=5  
-i=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full  
-iflat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed  
-idark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed  
--region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]'  
--rotation=1  
--max-angle=360.000000  
--find-cor -t=tomopy  
--output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000
```

>python tomo_main.py -n=5 -i=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full -iflat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed -idark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' --rotation=1 --max-angle=360.000000 --find-cor -t=tomopy --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000

EXPECTED RESULTS:

> COR: 136.0 <br>
> Memory usage: 175484 KB, 171.37109375 MB <br>

<br />

# Run Reconstruction

## Single Image <br>

```python
python tomo_main.py
--tool=tomopy
--algorithm=gridrec
--num-iter=5
--input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_stack_larmor_summed
--input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed
--input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed
--region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]'
--output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000
--median-filter-size=3
--cor=255.000000
--rotation=1
--max-angle=360.000000
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_stack_larmor_summed --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --data-as-stack

---

## Full `RB000888 test stack larmor summed 201510`, bolts crop, good air region
- ### Full `RB000888_test_stack_larmor_summed_201510` dataset <br>
- ### **OUT OF BOUNDS** Air Region if `--crop-before-normalise` is SPECIFIED <br>
- ### Better results/Air Region if run wihout `--crop-before-normalise`

```python
python tomo_main.py 
--tool=tomopy
--algorithm=gridrec
--num-iter=5
--input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full
--input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed
--input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed
--region-of-interest='[41.0, 0.0, 233.0, 228.0]'
--output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000
--median-filter-size=3
--cor=104.000000
--rotation=1
--max-angle=360.000000
--air-region='[360.0, 111.0, 388.0, 144.0]'
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[41.0, 0.0, 230.0, 228.0]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=104.000000 --rotation=1 --max-angle=360.000000 --air-region='[360.0, 111.0, 388.0, 144.0]' --data-as-stack

---

## Full `RB000888 test stack larmor summed 201510`, spheres crop, good air region
- ### Full `RB000888_test_stack_larmor_summed_201510` dataset <br>
- ### **OUT OF BOUNDS** Air Region if `--crop-before-normalise` is SPECIFIED <br>
- ### Better results/Air Region if run wihout `--crop-before-normalise`

```python
python tomo_main.py 
--tool=tomopy
--algorithm=gridrec
--num-iter=5
--input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full
--input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed
--input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed
--region-of-interest='[35.0, 232.0, 224.0, 509.0]'
--output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000
--median-filter-size=3
--cor=136.000000
--rotation=1
--max-angle=360.000000
--air-region='[360.0, 111.0, 388.0, 144.0]'
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[35.0, 232.0, 224.0, 509.0]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --air-region='[360.0, 111.0, 388.0, 144.0]' --data-as-stack
---

## Full `RB000888_test_stack_larmor_summed_201510` dataset, bolts crop, **BAD** air region
- ### ROI Crop **[36, 0, 219, 229]** <br>
- ### **WORKING** Air Region **[189.000000, 100.000000, 209.000000, 135.000000]** for crop if `--crop-before-normalise` is SPECIFIED <div id='id-section1'/>

```python
python tomo_main.py
--tool=tomopy
--algorithm=gridrec
--num-iter=5
--input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full
--input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed
--input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed
--region-of-interest='[35.0, 232.0, 224.0, 509.0]'
--output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000
--median-filter-size=3
--cor=104.0
--rotation=1
--max-angle=360.000000
--air-region='[189.000000, 100.000000, 209.000000, 135.000000]'
--crop-before-normalise --data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[35.0, 232.0, 224.0, 509.0]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=104.0 --rotation=1 --max-angle=360.000000 --air-region='[189.000000, 100.000000, 209.000000, 135.000000]' --crop-before-normalise --data-as-stack

<br/>

# ImageJ `GetSelectionCoordinates` Macro
- ### Gets selection coordinates and prints them in appropriate format to be copy pasted into Terminal

```
macro "List XY Coordinates" {
  requires("1.30k");
  getSelectionCoordinates(x, y);
  print("\'["+x[0]+".0, "+y[1]+".0, "+x[1]+".0, "+y[2]+".0]\'")
}
```
<br/>

# Python local tests

## Pyfits load image stack
```python
import pyfits
pyfits.open('/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/gridrec/pre_processed/out_preproc_proj_images_stack.fits')
```

## Test loading single images and image stack
```python
from recon.data import loader
import numpy as np

# load single images
sample = loader.read_stack_of_images('/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full', argument_data_dtype=np.float32)[0]

# load a stack of images
sample = loader.read_stack_of_images('/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/gridrec/pre_processed', argument_data_dtype=np.float32)[0]
```

# Astra Reconstructions