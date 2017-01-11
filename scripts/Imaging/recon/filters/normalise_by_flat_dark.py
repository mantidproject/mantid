def execute(self, data, cfg, norm_flat_img, norm_dark_img):
    """
    Normalize by flat and dark images

    @param data :: image stack as a 3d numpy array
    @param cfg :: pre-processing configuration
    @param norm_flat_img :: flat (open beam) image to use in normalization
    @param norm_dark_img :: dark image to use in normalization

    Returns :: filtered data (stack of images)
    """
    self._check_data_stack(data)

    if not cfg or not isinstance(cfg, tomocfg.PreProcConfig):
        raise ValueError(
            "Cannot normalize by flat/dark images without a valid pre-processing "
            "configuration")

    if not cfg.normalize_flat_dark:
        self.tomo_print(
            " * Note: NOT applying normalization by flat/dark images.",
            verbosity=2)
        return data

    if isinstance(norm_flat_img, np.ndarray):
        if 2 != len(norm_flat_img.
                            shape) or norm_flat_img.shape != data.shape[1:]:
            raise ValueError(
                "Incorrect shape of the flat image ({0}) which should match the "
                "shape of the sample images ({1})".format(
                    norm_flat_img.shape, data[0].shape))
        self.tomo_print_timed_start(
            " * Starting normalization by flat/dark images with pixel data type: {0}...".
                format(data.dtype))
        norm_divide = None
        if norm_dark_img:
            norm_divide = norm_flat_img - norm_dark_img
        else:
            norm_divide = norm_flat_img

        if self.crop_before_normaliz and cfg.crop_coords:
            norm_divide = norm_divide[:, cfg.crop_coords[
                1]:cfg.crop_coords[3] + 1, cfg.crop_coords[0]:
                          cfg.crop_coords[2] + 1]
        # prevent divide-by-zero issues
        norm_divide[norm_divide == 0] = 1e-6

        if not norm_dark_img:
            norm_dark_img = 0
        for idx in range(0, data.shape[0]):
            data[idx, :, :] = np.true_divide(
                data[idx, :, :] - norm_dark_img, norm_divide)
        # true_divide produces float64, we assume that precision not needed (definitely not
        # for 16-bit depth output images as we usually have).
        self.tomo_print_timed_stop(
            " * Finished normalization by flat/dark images with pixel data type: {0}.".
                format(data.dtype))
    else:
        self.tomo_print(
            " * Note: cannot apply normalization by flat/dark images because no valid flat image has been "
            "provided in the inputs. Flat image given: {0}".format(
                norm_flat_img),
            verbosity=2)

    return data