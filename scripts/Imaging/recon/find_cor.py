from tomo_helper import Helper


class FindCOR(object):

    def find_center(self, cfg):
        self._check_paths_integrity(cfg)
        tomo_helper = Helper()
        tomo_helper.tomo_print_timed_start(
            " * Importing tool " + cfg.alg_cfg.tool)
        # import tool
        import tomorec.tool_imports as tti
        tomopy = tti.import_tomo_tool(cfg.alg_cfg.tool)
        tomo_helper.tomo_print_timed_stop(" * Tool loaded.")

        tomo_helper.tomo_print_timed_start(" * Loading data...")
        # load in data
        sample, white, dark = self.read_in_stack(
            cfg.preproc_cfg.input_dir, cfg.preproc_cfg.in_img_format,
            cfg.preproc_cfg.input_dir_flat, cfg.preproc_cfg.input_dir_dark)
        tomo_helper.tomo_print_timed_stop(
            " * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
                sample.shape, sample.dtype))

        # rotate
        sample, white, dark = self.rotate_stack(sample, cfg.preproc_cfg)

        # crop the ROI, this is done first, so beware of what the correct ROI
        # coordinates are
        sample = self.crop_coords(sample, cfg.preproc_cfg)

        # sanity check
        tomo_helper.tomo_print(" * Sanity check on data", 0)
        self._check_data_stack(sample)

        num_projections = sample.shape[0]
        inc = float(cfg.preproc_cfg.max_angle) / (num_projections - 1)

        tomo_helper.tomo_print(" * Calculating projection angles")
        proj_angles = np.arange(0, num_projections * inc, inc)
        # For tomopy
        tomo_helper.tomo_print(" * Calculating radians for TomoPy")
        proj_angles = np.radians(proj_angles)

        size = int(num_projections)

        # depending on the number of COR projections it will select different
        # slice indices
        cor_num_checked_projections = 6
        cor_proj_slice_indices = []
        cor_slice_index = 0

        if cor_num_checked_projections < 2:
            # this will give us the middle slice
            cor_slice_index = int(size / 2)
            cor_proj_slice_indices.append(cor_slice_index)
        else:
            for c in range(cor_num_checked_projections):
                cor_slice_index += int(size / cor_num_checked_projections)
                cor_proj_slice_indices.append(cor_slice_index)

        calculated_cors = []

        tomo_helper.tomo_print_timed_start(
            " * Starting COR calculation for " +
            str(cor_num_checked_projections) + " out of " +
            str(sample.shape[0]) + " projections", 2)

        cropCoords = cfg.preproc_cfg.crop_coords[0]
        imageWidth = sample.shape[2]

        # if crop corrds match with the image width then the full image was
        # selected
        pixelsFromLeftSide = cropCoords if cropCoords - imageWidth <= 1 else 0

        for slice_idx in cor_proj_slice_indices:
            tomopy_cor = tomopy.find_center(
                tomo=sample, theta=proj_angles, ind=slice_idx, emission=False)
            print(" ** COR for slice", str(slice_idx), ".. REL to CROP ",
                  str(tomopy_cor), ".. REL to FULL ",
                  str(tomopy_cor + pixelsFromLeftSide))
            calculated_cors.append(tomopy_cor)

        tomo_helper.tomo_print_timed_stop(" * Finished COR calculation.", 2)

        averageCORrelativeToCrop = sum(calculated_cors) / len(calculated_cors)
        averageCORrelativeToFullImage = sum(calculated_cors) / len(
            calculated_cors) + pixelsFromLeftSide

        # we add the pixels cut off from the left, to reflect the full image in
        # Mantid
        tomo_helper.tomo_print(" * Printing average COR in relation to cropped image "
                               + str(cfg.preproc_cfg.crop_coords) + ":", 2)
        print(str(round(averageCORrelativeToCrop)))
        tomo_helper.tomo_print(" * Printing average COR in relation to FULL image:",
                               2)
        print(str(round(averageCORrelativeToFullImage)))
