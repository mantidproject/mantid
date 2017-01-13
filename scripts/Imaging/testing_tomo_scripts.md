## Merge pre-processing images into a stack. This will not apply any filters as pre-processing, it will just pack all of the images into a stack

```python
python tomo_main.py  
-i=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full  
-o=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000  
--preproc-as-stack
```
For Copy/Paste to terminal:
>python tomo_main.py -i=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full -o=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --preproc-as-stack

---
## Run Find_Center with a single image and no crop

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
## Run Find_Center with full `RB000888_test_stack_larmor_summed_201510` and crop **[36, 227, 219, 510]**

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
- ### Single Image <br>
- ### ROI Crop **[36, 227, 219, 510]** <br>

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
--preproc-as-stack --save-preproc
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_stack_larmor_summed --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[36.000000, 227.000000, 219.000000, 510.000000]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --preproc-as-stack --save-preproc

---

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
--cor=255.000000
--rotation=1
--max-angle=360.000000
--air-region='[360.0, 111.0, 388.0, 144.0]'
--preproc-as-stack --save-preproc
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[41.0, 0.0, 230.0, 228.0]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --air-region='[360.0, 111.0, 388.0, 144.0]' --preproc-as-stack --save-preproc
---

- ### Full `RB000888_test_stack_larmor_summed_201510` dataset<br>
- ### ROI Crop **[36, 0, 219, 229]** <br>
- ### **WORKING** Air Region **[189.000000, 100.000000, 209.000000, 135.000000]** for crop if `--crop-before-normalise` is SPECIFIED <br>

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
--cor=255.000000
--rotation=1
--max-angle=360.000000
--air-region='[189.000000, 100.000000, 209.000000, 135.000000]'
--crop-before-normalise --preproc-as-stack --save-preproc
```
For Copy/Paste to terminal:
>python tomo_main.py --tool=tomopy --algorithm=gridrec --num-iter=5 --input-path=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/data_full --input-path-flat=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed --input-path-dark=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed --region-of-interest='[35.0, 232.0, 224.0, 509.0]' --output=/media/matt/Windows/Documents/mantid_workspaces/imaging/RB000888_test_stack_larmor_summed_201510/processed/reconstruction_TomoPy_gridrec_2016December15_093530_690212000 --median-filter-size=3 --cor=255.000000 --rotation=1 --max-angle=360.000000 --air-region='[189.000000, 100.000000, 209.000000, 135.000000]' --crop-before-normalise --preproc-as-stack --save-preproc

ImageJ `GetSelectionCoordinates` Macro:

```
macro "List XY Coordinates" {
  requires("1.30k");
  getSelectionCoordinates(x, y);
  print("\'["+x[0]+".0, "+y[1]+".0, "+x[1]+".0, "+y[2]+".0]\'")
}
```