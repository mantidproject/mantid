def read_in_stack(self,
                  sample_path,
                  img_format,
                  flat_field_path=None,
                  dark_field_path=None):
    """
    Loads a stack, including sample, white and dark images.

    @param sample_path :: path to sample images

    @param img_format :: image format to expect/use (as a filename extension)

    @param flat_field_path :: (optional) path to open beam / white image(s).
    Can be a file or directory

    @param dark_field_path :: (optional) path to dark field image(s).
    Can be a file or directory

    Returns :: stack of images as a 3-elements tuple: numpy array with sample images, white image,
    and dark image.
    """
    # Note, not giving prefix. It will load all the files found.
    # Example prefixes are prefix = 'tomo_', prefix = 'LARMOR00', prefix =
    # 'angle_agg'

    sample, white, dark = tomoio.read_stack_of_images(
        sample_path,
        flat_field_path=flat_field_path,
        dark_field_path=dark_field_path,
        file_extension=img_format)

    if not isinstance(sample, np.ndarray) or not sample.shape \
            or not isinstance(sample.shape, tuple) or 3 != len(sample.shape):
        raise RuntimeError(
            "Error reading sample images. Could not produce a 3-dimensional array "
            "of data from the sample images. Got: {0}".format(sample))

    return (sample, white, dark)
