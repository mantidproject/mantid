# Available pre-processing of input raw images/projections:
## Region of interest
```
  -R [REGION_OF_INTEREST [REGION_OF_INTEREST ...]], --region-of-interest [REGION_OF_INTEREST [REGION_OF_INTEREST ...]]
                        Crop original images using these coordinates. The selection is a rectangle and expected order is - Left Top Right Bottom.
                        If not given, the whole images are used.
                        Example: --region-of-interest 150 234 23 22
```
## Air Region
```
  -A [AIR_REGION [AIR_REGION ...]], --air-region [AIR_REGION [AIR_REGION ...]]
                        Air region /region for normalisation. The selection is a rectangle and expected order is - Left Top Right Bottom.
                        For best results the region selected should not be blocked by any object in the Tomography.
                        Example: --air-region 150 234 23 22
```
## Median Filter
```
  --pre-median-size PRE_MEDIAN_SIZE
                        Size / width of the median filter(pre - processing).
  --pre-median-mode {reflect,constant,nearest,mirror,wrap}
                        Default: reflect
                        Mode of median filter which determines how the array borders are handled.
```
## Remove stripes
```
  --pre-stripe-removal-wf [PRE_STRIPE_REMOVAL_WF [PRE_STRIPE_REMOVAL_WF ...]]
                        Stripe removal using wavelett-fourier method. Available parameters:
                                level (int, optional) Number of discrete wavelet transform levels.
                                wname (str, optional) Type of the wavelet filter. 'haar', 'db5', 'sym5', etc.
                                sigma (float, optional) Damping parameter in Fourier space.
                                pad (bool, optional) If True, extend the size of the sinogram by padding with zeros.
  --pre-stripe-removal-ti [PRE_STRIPE_REMOVAL_TI [PRE_STRIPE_REMOVAL_TI ...]]
                        Stripe removal using Titarenko's approach. Available parameters:
                                      nblock (int, optional) Number of blocks.
                                      alpha (int, optional) Damping factor.
  --pre-stripe-removal-sf [PRE_STRIPE_REMOVAL_SF [PRE_STRIPE_REMOVAL_SF ...]]
                        Stripe removal using smoothing-filter method. Available parameters:
                                size (int, optional) Size of the smoothing filter.
```

## Rotation
```
  -r ROTATION, --rotation ROTATION
                        Rotate images by 90 degrees a number of times.
                        The rotation is clockwise unless a negative number is given which indicates rotation counterclockwise.
```
## Clip values after flat/dark normalisation
```
  --clip-min CLIP_MIN   Default: 0.0
                        Clip values below this after normalisations to remove out of bounds pixel values.
  --clip-max CLIP_MAX   Default: 1.5
                        Clip values above this after normalisations to remove out of bounds pixel values.
```
## Cut-off **bright** pixels
```
  --cut-off CUT_OFF     Default: None
                        Cut off values above threshold relative to the max pixels.
```
## Remove outliers
```
  --pre-outliers PRE_OUTLIERS
                        Crop bright pixels.
  --pre-outliers-radius PRE_OUTLIERS_RADIUS
                        Radius for the median filter to determine the outlier.
```
## Rebin
```
  --rebin REBIN         Rebin factor by which the images will be rebinned. This could be any positive float number.
                        If not specified no scaling will be done.
  --rebin-mode {nearest,lanczos,bilinear,bicubic,cubic}
                        Default: bilinear
                        Specify which interpolation mode will be used for the scaling of the image.
```
## Gaussian Filter
```
  --pre-gaussian-size PRE_GAUSSIAN_SIZE
                        Apply gaussian filter (2d) on reconstructed volume with the given window size.
  --pre-gaussian-mode {reflect,constant,nearest,mirror,wrap}
                        Default: reflect
                        Mode of gaussian filter which determines how the array borders are handled.(pre processing).
  --pre-gaussian-order PRE_GAUSSIAN_ORDER
                        Default: 0
                        The order of the filter along each axis is given as a sequence of integers, 
                        or as a single number. An order of 0 corresponds to convolution with a Gaussian kernel.
                        An order of 1, 2, or 3 corresponds to convolution with the first, second or third derivatives of a Gaussian.
                        Higher order derivatives are not implemented.
```
## Minus Log
```
  -log, --pre-minus-log
                        Calculate the -log of the sample data.
```

# Post-processing of the reconstructed volume:
## Circular Mask
```
  --circular-mask CIRCULAR_MASK
                        Radius of the circular mask to apply on the reconstructed volume.
                        It is given in [0,1] relative to the size of the smaller dimension/edge of the slices.
                        Empty or zero implies no masking.
  --circular-mask-val CIRCULAR_MASK_VAL
                        The value that the pixels in the mask will be set to.
```
## Post-processing Remove Outliers
```
  --post-outliers POST_OUTLIERS
                        Outliers threshold for reconstructed volume.
                        Pixels below and/or above (depending on mode) this threshold will be clipped.
  --post-outliers-radius POST_OUTLIERS_RADIUS
                        Radius for the median filter to determine the outlier.
```
## Median Filter
```
  --post-median-size POST_MEDIAN_SIZE
                        Apply median filter (2d) on reconstructed volume with the given window size.(post processing)
  --post-median-mode {reflect,constant,nearest,mirror,wrap}
                        Default: reflect
                        Mode of median filter which determines how the array borders are handled.(post processing)
```
## Gaussian Filter
```
  --post-gaussian-size POST_GAUSSIAN_SIZE
                        Apply gaussian filter (2d) on reconstructed volume with the given window size.
  --post-gaussian-mode {reflect,constant,nearest,mirror,wrap}
                        Default: reflect
                        Mode of gaussian filter which determines how the array borders are handled.(post processing).
  --post-gaussian-order POST_GAUSSIAN_ORDER
                        Default: 0
                        The order of the filter along each axis is given as a sequence of integers, 
                        or as a single number. An order of 0 corresponds to convolution with a Gaussian kernel.
                        An order of 1, 2, or 3 corresponds to convolution with the first, second or third derivatives of a Gaussian.
                        Higher order derivatives are not implemented.
```
## Ring Removal
```
  --ring-removal        Perform Ring Removal on the post processed data.
  --ring-removal-x RING_REMOVAL_X
                        Abscissa location of center of rotation
  --ring-removal-y RING_REMOVAL_Y
                        Ordinate location of center of rotation
  --ring-removal-thresh RING_REMOVAL_THRESH
                        Maximum value of an offset due to a ring artifact
  --ring-removal-thresh-max RING_REMOVAL_THRESH_MAX
                        Max value for portion of image to filter
```