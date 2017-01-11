def run_reconstruct_3d(self,
                       proj_data,
                       preproc_cfg,
                       alg_cfg,
                       reconstruction_tool=None): # remove reconstruction_tool, this is tomopy specific file

    self._check_data_stack(proj_data)

    num_proj = proj_data.shape[0]
    inc = float(preproc_cfg.max_angle) / (num_proj - 1)

    proj_angles = np.arange(0, num_proj * inc, inc)
    # For tomopy
    proj_angles = np.radians(proj_angles)

    if 'astra' == alg_cfg.tool:
        # run_reconstruct_3d_astra(proj_data, algorithm, cor, proj_angles=proj_angles)
        return self.run_reconstruct_3d_astra_simple(
            proj_data, proj_angles, alg_cfg, preproc_cfg.cor,
            reconstruction_tool)

    self.tomo_print(" * Using center of rotation: {0}".format(
        preproc_cfg.cor))
    if 'tomopy' == alg_cfg.tool and 'gridrec' != alg_cfg.algorithm and 'fbp' != alg_cfg.algorithm:
        if not alg_cfg.num_iter:
            reconstruction_tool.cfg_num_iter = tomocfg.PreProcConfig.DEF_NUM_ITER
        # For ref, some typical run times with 4 cores:
        # 'bart' with num_iter=20 => 467.640s ~= 7.8m
        # 'sirt' with num_iter=30 => 698.119 ~= 11.63
        self.tomo_print_timed_start(
            " * Starting iterative method with TomoPy. Algorithm: {0}, "
            "number of iterations: {1}...".format(alg_cfg.algorithm,
                                                  alg_cfg.num_iter))
        rec = reconstruction_tool.recon(
            tomo=proj_data,
            theta=proj_angles,
            center=preproc_cfg.cor,
            algorithm=alg_cfg.algorithm,
            num_iter=alg_cfg.num_iter)  # , filter_name='parzen')

    else:
        self.tomo_print_timed_start(
            " * Starting non-iterative reconstruction algorithm with TomoPy. "
            "Algorithm: {0}...".format(alg_cfg.algorithm))
        rec = reconstruction_tool.recon(
            tomo=proj_data,
            theta=proj_angles,
            center=preproc_cfg.cor,
            algorithm=alg_cfg.algorithm)

    self.tomo_print_timed_stop(
        " * Reconstructed 3D volume. Shape: {0}, and pixel data type: {1}.".
            format(rec.shape, rec.dtype))

    return rec