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
- [Plot Circular Mask](#plot-circular-mask)
- [Normalise by background comparison](#normalise-by-background-comparison)
- [Astra Reconstructions](#astra-reconstructions)
- [Wrong tool/algorithm tests](#wrong-toolalgorithm-tests)
- [SciPy ndimage zoom](#scipy-ndimage-zoom)
- [SciPy misc imresize](#scipy-misc-imresize)
- [SciPy timeit misc.imresize vs ndimage.zoom](#scipy-timeit-miscimresize-vs-ndimagezoom)
  - [Bigger data test](#bigger-data-test)

<!-- /TOC -->
 

# Utility
## Merge pre-processing images into a stack. This will not apply any filters as pre-processing, it will just pack all of the images into a stack

```python
python tomo_main.py  
-i=~/Documents/img/000888/data_full  
-o=~/Documents/img/000888/processed/temp/1  
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py -i=~/Documents/img/000888/data_full -o=~/Documents/img/000888/processed/temp/1 --data-as-stack


<br/>

# Find Center Runs

## Single image and no crop

```python
python tomo_main.py  
--num-iter=5  
--input-path=~/Documents/img/000888/data_single  
--input-path-flat=~/Documents/img/000888/flat  
--input-path-dark=~/Documents/img/000888/dark  
--region-of-interest='[0.000000, 0.000000, 511.000000, 511.000000]'  
--rotation=1  
--max-angle=360.000000  
--find-cor  
--output=~/Documents/img/000888/processed/temp/1  
--tool=tomopy 
```

For Copy/Paste to terminal:
>python tomo_main.py --num-iter=5 --input-path=~/Documents/img/000888/data_single --input-path-flat=~/Documents/img/000888/flat --input-path-dark=~/Documents/img/000888/dark --region-of-interest='[0.000000, 0.000000, 511.000000, 511.000000]' --rotation=1 --max-angle=360.000000 --find-cor --output=~/Documents/img/000888/processed/temp/1 --tool=tomopy 

EXPECTED RESULTS:
> COR: 265.0

---

## Full `RB000888 test stack larmor summed 201510`, spheres crop

```python
python tomo_main.py  
-n=5  
-i=~/Documents/img/000888/data_full  
-iflat=~/Documents/img/000888/flat  
-idark=~/Documents/img/000888/dark  
--region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]'  
--rotation=1  
--max-angle=360.000000  
--find-cor -t=tomopy  
--output=~/Documents/img/000888/processed/temp/1
```

>python tomo_main.py -n=5 -i=~/Documents/img/000888/data_full -iflat=~/Documents/img/000888/flat -idark=~/Documents/img/000888/dark --region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' --rotation=1 --max-angle=360.000000 --find-cor -t=tomopy --output=~/Documents/img/000888/processed/temp/1

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
--input-path=~/Documents/img/000888/data_single
--input-path-flat=~/Documents/img/000888/flat
--input-path-dark=~/Documents/img/000888/dark
--region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]'
--output=~/Documents/img/000888/processed/temp/1
--median-filter-size=3
--cor=255.000000
--rotation=1
--max-angle=360.000000
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=~/Documents/img/000888/data_single --input-path-flat=~/Documents/img/000888/flat --input-path-dark=~/Documents/img/000888/dark --region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' --output=~/Documents/img/000888/processed/temp/1 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --data-as-stack

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
--input-path=~/Documents/img/000888/data_full
--input-path-flat=~/Documents/img/000888/flat
--input-path-dark=~/Documents/img/000888/dark
--region-of-interest='[41.0, 0.0, 233.0, 228.0]'
--output=~/Documents/img/000888/processed/temp/1
--median-filter-size=3
--cor=104.000000
--rotation=1
--max-angle=360.000000
--air-region='[360.0, 111.0, 388.0, 144.0]'
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=~/Documents/img/000888/data_full --input-path-flat=~/Documents/img/000888/flat --input-path-dark=~/Documents/img/000888/dark --region-of-interest='[41.0, 0.0, 230.0, 228.0]' --output=~/Documents/img/000888/processed/temp/1 --median-filter-size=3 --cor=104.000000 --rotation=1 --max-angle=360.000000 --air-region='[360.0, 111.0, 388.0, 144.0]' --data-as-stack

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
--input-path=~/Documents/img/000888/data_full
--input-path-flat=~/Documents/img/000888/flat
--input-path-dark=~/Documents/img/000888/dark
--region-of-interest='[35.0, 232.0, 224.0, 509.0]'
--output=~/Documents/img/000888/processed/temp/1
--median-filter-size=3
--cor=136.000000
--rotation=1
--max-angle=360.000000
--air-region='[360.0, 111.0, 388.0, 144.0]'
--data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=~/Documents/img/000888/data_full --input-path-flat=~/Documents/img/000888/flat --input-path-dark=~/Documents/img/000888/dark --region-of-interest='[35.0, 232.0, 224.0, 509.0]' --output=~/Documents/img/000888/processed/temp/1 --median-filter-size=3 --cor=136.000000 --rotation=1 --max-angle=360.000000 --air-region='[360.0, 111.0, 388.0, 144.0]' --data-as-stack
---

## Full `RB000888_test_stack_larmor_summed_201510` dataset, bolts crop, **BAD** air region
- ### ROI Crop **[36, 0, 219, 229]** <br>
- ### **WORKING** Air Region **[189.000000, 100.000000, 209.000000, 135.000000]** for crop if `--crop-before-normalise` is SPECIFIED <div id='id-section1'/>

```python
python tomo_main.py
--tool=tomopy
--algorithm=gridrec
--num-iter=5
--input-path=~/Documents/img/000888/data_full
--input-path-flat=~/Documents/img/000888/flat
--input-path-dark=~/Documents/img/000888/dark
--region-of-interest='[35.0, 232.0, 224.0, 509.0]'
--output=~/Documents/img/000888/processed/temp/1
--median-filter-size=3
--cor=104.0
--rotation=1
--max-angle=360.000000
--air-region='[189.000000, 100.000000, 209.000000, 135.000000]'
--crop-before-normalise --data-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=~/Documents/img/000888/data_full --input-path-flat=~/Documents/img/000888/flat --input-path-dark=~/Documents/img/000888/dark --region-of-interest='[35.0, 232.0, 224.0, 509.0]' --output=~/Documents/img/000888/processed/temp/1 --median-filter-size=3 --cor=104.0 --rotation=1 --max-angle=360.000000 --air-region='[189.000000, 100.000000, 209.000000, 135.000000]' --crop-before-normalise --data-as-stack

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
import matplotlib.pyplot as plt
from recon.filters import rotate_stack

# load single images
sample = loader.read_stack_of_images('~/Documents/img/000888/data_full', argument_data_dtype=np.float32)[0]
rsample = rotate_stack._rotate_stack(sample, 3)
plt.imshow(rsample[0,232:509,35:224], cmap='Greys_r')  # spheres
plt.show()

plt.imshow(rsample[0,0:228,41:233], cmap='Greys_r')  # bolts
plt.show()

csample = rsample[:, 0:228,41:233]
sinograms = np.swapaxes(csample, 0, 1)

plt.imshow(rsample[:,0,:], cmap='Greys_r') # sinogram
plt.show()


import tomopy 
plt.imshow(tomopy.circ_mask(csample, axis=0, ratio=0.98)[0, :, :]); plt.show()
# load a stack of images
sample = loader.read_stack_of_images('/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/gridrec/pre_processed', argument_data_dtype=np.float32)[0]
```

# Plot Circular Mask
```python
from recon.data import loader
import numpy as np
import matplotlib.pyplot as plt
from recon.filters import rotate_stack

# load single images
sample = loader.read_stack_of_images('~/Documents/img/000888/data_full', argument_data_dtype=np.float32)[0]
rsample = rotate_stack._rotate_stack(sample, 3)
csample = rsample[:, 0:228,41:233]
import tomopy 
plt.imshow(tomopy.circ_mask(csample, axis=0, ratio=0.98)[0], cmap='Greys_r'); plt.show()
from recon.filters import circular_mask
plt.imshow(circular_mask.execute_custom(csample, 0.98)[0], cmap='Greys_r'); plt.show()
```

# Normalise by background comparison
```python
from recon.data import loader
from recon.filters import normalise_by_flat_dark
from recon.filters import rotate_stack as rs
import numpy as np
import matplotlib.pyplot as plt
import tomopy
from recon.configs.recon_config import ReconstructionConfig

config = ReconstructionConfig.emtpy_init()

sample, flat, dark = loader.read_stack_of_images(sample_path='~/Documents/img/000888/data_full', flat_file_path='~/Documents/img/000888/flat', dark_file_path='~/Documents/img/000888/dark', argument_data_dtype=np.float32)
tsample, tflat, tdark = loader.read_stack_of_images(sample_path='~/Documents/img/000888/data_full', flat_file_path='~/Documents/img/000888/flat', dark_file_path='~/Documents/img/000888/dark', argument_data_dtype=np.float32)
r = 3

sample = rs._rotate_stack(sample, r)
tsample = rs._rotate_stack(tsample, r)
flat = rs._rotate_image(flat, r)
tflat = rs._rotate_image(tflat, r)
dark = rs._rotate_image(dark, r)
tdark = rs._rotate_image(tdark, r)

sample = normalise_by_flat_dark.execute(sample, config, flat, dark)
tsample = tomopy.normalize(tsample, flat, dark)

plt.imshow(np.concatenate((sample[0], tsample[0]), axis=1), cmap='Greys_r'); plt.show()

```

# Astra Reconstructions
`astra_create_proj_geom`:
- det_spacing: distance between the centers of two adjacent detector pixels (0.55?)
- det_count: number of detector pixels in a single projection (512?)
- angles: projection angles in radians

```python
from recon.data import loader
import numpy as np
import matplotlib.pyplot as plt
from recon.filters import rotate_stack

# load single images
sample = loader.read_stack_of_images('~/Documents/img/000888/data_full', argument_data_dtype=np.float32)[0]
rsample = rotate_stack._rotate_stack(sample, 3)
csample = rsample[:, 0:228,41:233]
num_proj = csample.shape[0]
inc = float(360.0) / num_proj
proj_angles = np.arange(0, num_proj * inc, inc)
proj_angles = np.radians(proj_angles)

vol_geom = astra.create_vol_geom(512)
proj_geom = astra.create_proj_geom('parallel', 0.55, 512, proj_angles)

```

# Wrong tool/algorithm tests

```python
python tomo_main.py 
-i=~/Documents/img/000888/data_single 
--input-path-flat=~/Documents/img/000888/flat 
--input-path-dark=~/Documents/img/000888/dark 
--region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' 
-o=~/Documents/img/000888/processed/temp/1 
--median-filter-size=3 
--cor=255.000000 
--rotation=1 
--max-angle=360.000000 
--data-as-stack 
-t tomopy # just change tool
-a afewaf # or or algorithm
```

> python tomo_main.py -i=~/Documents/img/000888/data_single --input-path-flat=~/Documents/img/000888/flat --input-path-dark=~/Documents/img/000888/dark --region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' -o=~/Documents/img/000888/processed/temp/1 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --data-as-stack -t tomopy -a afewaf


# SciPy ndimage zoom
```python
from recon.data import loader
import numpy as np
import matplotlib.pyplot as plt
from recon.filters import rotate_stack
import scipy.ndimage as sn

# load single images
sample = loader.read_stack_of_images('~/Documents/img/000888/data_full', argument_data_dtype=np.float32)[0]
rsample = rotate_stack._rotate_stack(sample, 3)
print(rsample.shape)
scale = 0.6565
num_images = rsample.shape[0]
expected_dims = round(rsample.shape[1]*scale)  # this will give the shape calculated by scipy
boop = np.zeros((num_images, expected_dims, expected_dims), dtype=np.float32)
print(boop.shape) 
for idx in xrange(rsample.shape[0]):
    boop[idx] = sn.zoom(rsample[idx], scale)
    rsample[idx] = 0

plt.imshow(boop[0], cmap='Greys_r'); plt.show()
```

# SciPy misc imresize
```python
from recon.data import loader
import numpy as np
import matplotlib.pyplot as plt
from recon.filters import rotate_stack
import scipy.misc as sm
# load single images
sample = loader.read_stack_of_images('~/Documents/img/000888/data_full', argument_data_dtype=np.float32)[0]
rsample = rotate_stack._rotate_stack(sample, 3)
print(rsample.shape)
scale = 0.6565
num_images = rsample.shape[0]
expected_dims = round(rsample.shape[1]*scale)  # this will give the shape calculated by scipy
boop = np.zeros((num_images, expected_dims, expected_dims), dtype=np.float32)
print(boop.shape) 
for idx in xrange(rsample.shape[0]):
    boop[idx] = sm.imresize(rsample[idx], scale, interp='nearest')
    rsample[idx] = 0

plt.imshow(boop[0], cmap='Greys_r'); plt.show()

```

# SciPy timeit misc.imresize vs ndimage.zoom

```python
from recon.data import loader
import numpy as np
import matplotlib.pyplot as plt
from recon.filters import rotate_stack
import scipy.misc as sm
import scipy.ndimage as sn
from recon.helper import Helper

sample = loader.read_stack_of_images("~/Documents/img/000888/data_full")[0]

def imresize(sample):
    boop = np.zeros((143, 336, 336), dtype=np.float32)
    
    for idx in xrange(143):
        boop[idx] = sm.imresize(sample[idx], 0.6565, interp='nearest')
        sample[idx] = 0


def zoom(sample):
    
    boop = np.zeros((143, 336, 336), dtype=np.float32)
    for idx in xrange(143):
        boop[idx] = sn.zoom(sample[idx], 0.6565)


import timeit
timeit.timeit(stmt='imresize(sample)', setup='from __main__ import sample, loader, imresize; import numpy as np; gc.enable()', number=100)
timeit.timeit(stmt='zoom(sample)', setup='from __main__ import sample, loader, zoom; import numpy as np; gc.enable()', number=100)
```

## Bigger data test
```python

from recon.data import loader
import numpy as np
import matplotlib.pyplot as plt
from recon.filters import rotate_stack
import scipy.misc as sm
import scipy.ndimage as sn
from recon.helper import Helper

sample = loader.read_stack_of_images("~/Documents/img/000888/data_full")[0]
sample = np.concatenate(sample, sample)
def imresize(sample):
    boop = np.zeros((143, 336, 336), dtype=np.float32)
    
    for idx in xrange(143):
        boop[idx] = sm.imresize(sample[idx], 0.6565, interp='nearest')
        sample[idx] = 0


def zoom(sample):
    
    boop = np.zeros((143, 336, 336), dtype=np.float32)
    for idx in xrange(143):
        boop[idx] = sn.zoom(sample[idx], 0.6565)


import timeit
timeit.timeit(stmt='imresize(sample)', setup='from __main__ import sample, loader, imresize; import numpy as np; gc.enable()', number=100)
timeit.timeit(stmt='zoom(sample)', setup='from __main__ import sample, loader, zoom; import numpy as np; gc.enable()', number=100)
```