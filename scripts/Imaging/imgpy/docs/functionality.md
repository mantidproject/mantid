# Available functionality
## input formats
## output formats
#Functionality options:
## Input path
  -i INPUT_PATH, --input-path INPUT_PATH
                        Input directory
## Input path for flat images
  -F INPUT_PATH_FLAT, --input-path-flat INPUT_PATH_FLAT
                        Input directory for flat images
## Input path for dark images
  -D INPUT_PATH_DARK, --input-path-dark INPUT_PATH_DARK
                        Input directory for flat images

## Input image format
  --in-format {nxs}     Format/file extension expected for the input images.

## Output path
  -o OUTPUT_PATH, --output-path OUTPUT_PATH
                        Where to write the output slice images (reconstructed volume).
## Output image format
  --out-format {nxs}    Format/file extension expected for the input images.

## Prefix for output reconstruction slices
  --out-slices-prefix OUT_SLICES_PREFIX
                        The prefix for the reconstructed slices files.
## Subdirectory for output horizontal reconstruction slices
  --out-horiz-slices-subdir OUT_HORIZ_SLICES_SUBDIR
                        The subdirectory for the reconstructed horizontal slices.
## Prefix for output horizontal reconstruction slices
  --out-horiz-slices-prefix OUT_HORIZ_SLICES_PREFIX
                        The prefix for the reconstructed horizontal slices files.

## Save pre-processed images
  -s, --save-preproc    Save out the pre-processed images.

## Do an only pre-processing run
  --only-preproc        Complete pre-processing of images and exit.

## Reuse pre-processing images. Skip any pre-processing, and go straight to reconstruction
  --reuse-preproc       The images loaded have already been pre-processed. All pre-processing steps will be skipped.

## Do an only post-processing run
  --only-postproc       The images have already been reconstructed. All pre-processing and reconstruciton steps will be skipped.

## Save out horizontal reconstruction slices
  --save-horiz-slices   Save out the horizontal reconstructed files.

## Subdirectory for the pre-processing slices
  -p PREPROC_SUBDIR, --preproc-subdir PREPROC_SUBDIR
                        The subdirectory for the pre-processed images.
                        Default output-path/pre_processed/.
## Save out pre-processing images as Radiograms.
  --swap_axes          NOT RECOMMENDED: This means an additional conversion will be done inside Tomopy, which will double the memory usage temporarily.
                        Pre-processed images will be saved as swap_axes if --save-preproc is specified.
                        If --reuse-preproc is specified, then the images that will be loaded will be expected to be swap_axes.
## Data type
  --data-dtype DATA_DTYPE
                        The data type in which the data will be processed.
                        Supported: float32, float64

## Centers of rotation for slices specified with --cor-slices
  -c [CORS [CORS ...]], --cors [CORS [CORS ...]]
                        Provide the CORs for the selected slices with --cor-slices.
                        If no slices are provided a SINGLE COR is expected, that will be used for the whole stack.
                        If slices are provided, the number of CORs provided with this option MUST BE THE SAME as the slices.

## Slice IDs for which the CORs are pre-calculated
  --cor-slices [COR_SLICES [COR_SLICES ...]]
                        Specify the Slice IDs to which the centers of rotation from --cors correspond.
                        The number of slices passed here MUST be the same as the number of CORs provided.
                        The slice IDs MUST be ints. If no slice IDs are provided, then only 1 COR is expected and will be used for the whole stack.
## Verbosity
  -v VERBOSITY, --verbosity VERBOSITY
                        0 - Silent, no text output at all, except results (not recommended)
                        1 - Low verbosity, will output text on step name
                        2 - Normal verbosity, will output step name and execution time
                        3 - High verbosity, will output step name, execution time and memory usage before and after each step
                        Default: 2 - Normal verbosity.
## Overwrite all conflicting files in directory.
  -w, --overwrite-all   Overwrite all conflicting files found in the output directory.

Run Modes:
## Convert images from one file format to another
  --convert             Convert images to a different format.

## Prefix for saved out files from conversion
  --convert-prefix CONVERT_PREFIX
                        Prefix for saved out files from conversion.

## Image operator mode
  --imopr [IMOPR [IMOPR ...]]
                        Image operator currently supports the following operators: ['recon', 'sino', 'show', 'vis', 'cor', 'corvo', 'corpc', 'corwrite', 'sum', '+', 'subtract', 'sub', '-', 'divide', 'div', '/', 'multiply', 'mul', '*', 'mean', 'avg', 'x']

## Aggregate energy levels
  --aggregate [AGGREGATE [AGGREGATE ...]]
                        Aggregate the selected image energy levels. The expected input is --aggregate <start> <end> <method:{sum, avg}>... to select indices.
                                          There must always be an even lenght of indices: --aggregate 0 100 101 201 300 400 sum
## Aggregate angles
  --aggregate-angles [AGGREGATE_ANGLES [AGGREGATE_ANGLES ...]]
                        Select which angles to be aggregated with --aggregate.
                        This can be used to spread out the load on multiple nodes.
                        Sample command: --aggregate-angles 0 10, will select only angles 0 - 10 inclusive.

## Aggregate single folder output
  --aggregate-single-folder-output
                        The output will be images with increasing number in a single folder.

## Debug, tries to connect to a remote debugger at the specified port with pydevd
  --debug               Run debug to specified port, if no port is specified, it will default to 59003

## Port on which to look for a debugger
  --debug-port DEBUG_PORT
                        Port on which a debugger is listening.

# Reconstruction options:
## Tool to be used for reconstruction
  -t {tomopy,astra}, --tool {tomopy,astra}
                        Tomographic reconstruction tool to use.
                        Available: ['tomopy', 'astra']

## Algorithm to be used for reconstruction
  -a ALGORITHM, --algorithm ALGORITHM
                        Reconstruction algorithm (tool dependent).
                        Available:
                        TomoPy: ['art', 'bart', 'fbp', 'gridrec', 'mlem', 'osem', 'ospml_hybrid', 'ospml_quad', 'pml_hybrid', 'pml_quad', 'sirt']
                        Astra:['FP', 'FP_CUDA', 'BP', 'BP_CUDA', 'FBP', 'FBP_CUDA', 'SIRT', 'SIRT_CUDA', 'SART', 'SART_CUDA', 'CGLS', 'CGLS_CUDA']

## Number of iterations for iterative algorithms
  -n NUM_ITER, --num-iter NUM_ITER
                        Number of iterations(only valid for iterative methods: {'art', 'bart', 'mlem', 'osem', 'ospml_hybrid', 'ospml_quad', pml_hybrid', 'pml_quad', 'sirt'}.
## Max Angle the angle of the last projection
  --max-angle MAX_ANGLE
                        Maximum angle of the last projection.
                        Assuming first angle=0, and uniform angle increment for every projection
## Cores
  --cores CORES         Number of CPU cores that will be used for reconstruction.
## Chunksize
  --chunksize CHUNKSIZE
                        How to spread the load on each worker.
## Parallel Load
  --parallel-load       Load the data with multiple reader processes. This CAN MAKE THE LOADING slower on a single local Hard Disk Drive.
