def execute(data, pre_cfg):
    """
    TODO move to filters.py
    Normalize by beam intensity. This is not directly about proton
    charg - not using the proton charge field as usually found in
    experiment/nexus files. This uses an area of normalization, if
    provided in the pre-processing configuration. TODO: much
    of this method should be moved into filters.

    @param data :: stack of images as a 3d numpy array
    @param pre_cfg :: pre-processing configuration

    Returns :: filtered data (stack of images)

    """
    self._check_data_stack(data)

    if not pre_cfg or not isinstance(pre_cfg, tomocfg.PreProcConfig):
        raise ValueError(
            "Cannot normalize by air region without a valid pre-processing configuration"
        )

    if pre_cfg.normalize_air_region:
        if not isinstance(pre_cfg.normalize_air_region, list) or \
                        4 != len(pre_cfg.normalize_air_region):
            raise ValueError(
                "Wrong air region coordinates when trying to use them to normalize images: {0}".
                    format(pre_cfg.normalize_air_region))

        if not all(
                isinstance(crd, int)
                for crd in pre_cfg.normalize_air_region):
            raise ValueError(
                "Cannot use non-integer coordinates to use the normalization region "
                "(air region). Got these coordinates: {0}".format(
                    pre_cfg.normalize_air_region))

        right = pre_cfg.normalize_air_region[2]  # why do we add 1?
        top = pre_cfg.normalize_air_region[1]
        left = pre_cfg.normalize_air_region[0]
        bottom = pre_cfg.normalize_air_region[3]

        # skip if for example: 0, 0, 0, 0 (empty selection)
        if top >= bottom or left >= right:
            self.tomo_print(
                " * NOTE: NOT applying Normalise by Air Region. Reason: Empty Selection"
            )
            return data

        self.tomo_print_timed_start(
            " * Starting normalization by air region. Statistics of values in the air region..."
        )
        air_sums = []
        for idx in range(0, data.shape[0]):
            air_data_sum = data[idx, top:bottom, left:right].sum()
            air_sums.append(air_data_sum)

        air_sums = np.true_divide(air_sums, np.amax(air_sums))

        self.tomo_print(
            " Air region sums (relative to maximum): " + air_sums,
            verbosity=0)

        for idx in range(0, data.shape[0]):
            data[idx, :, :] = np.true_divide(data[idx, :, :],
                                             air_sums[idx])

        avg = np.average(air_sums)
        self.tomo_print_timed_stop(
            " * Finished normalization by air region. Statistics of values in the air region, "
            "average: {0}, max ratio: {1}, min ratio: {2}.".format(
                avg, np.max(air_sums) / avg, np.min(air_sums) / avg))

    else:
        self.tomo_print(
            " * Note: NOT normalizing by air region", verbosity=2)

    return data