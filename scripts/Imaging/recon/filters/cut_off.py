def execute(data, config):
    # Apply cut-off for the normalization?
    if cfg.cut_off_level and cfg.cut_off_level > 0.0:
        self.tomo_print_timed_start(
            "* Applying cut-off with level: {0}".format(cfg.cut_off_level))
        dmin = np.amin(data)
        dmax = np.amax(data)
        rel_cut_off = dmin + cfg.cut_off_level * (dmax - dmin)
        data[data < rel_cut_off] = dmin
        self.tomo_print_timed_stop(
            " * Finished cut-off step, with pixel data type: {0}.".format(
                data.dtype))
    else:
        self.tomo_print(" * Note: NOT applying cut-off.", verbosity=2)

    return data
