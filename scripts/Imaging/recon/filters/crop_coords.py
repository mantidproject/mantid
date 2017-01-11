def execute(data, cfg):
    """
    Crop stack of images to a region (region of interest or similar), image by image

    @param data :: stack of images as a 3d numpy array
    @param cfg :: pre-processing configuration

    Returns :: filtered data (stack of images)
    """
    self._check_data_stack(data)

    # list with first-x, first-y, second-x, second-y
    if cfg.crop_coords:
        try:
            self.tomo_print_timed_start(
                " * Starting image cropping step, with pixel data type: {0}, coordinates: {1}. ...".
                    format(data.dtype, cfg.crop_coords))
            import prep as iprep
            data = iprep.filters.crop_vol(data, cfg.crop_coords)
            self.tomo_print_timed_stop(
                " * Finished image cropping step, with pixel data type: {0}, coordinates: {1}. "
                "Resulting shape: {2}.".format(data.dtype, cfg.crop_coords,
                                               data.shape))

        except ValueError as exc:
            print(
                "Error in crop (region of interest) parameter (expecting a list with four integers. "
                "Got: {0}. Error details: ".format(cfg.crop_coords), exc)
    else:
        self.tomo_print(
            " * Note: NOT applying cropping to region of interest.",
            verbosity=2)

    return data