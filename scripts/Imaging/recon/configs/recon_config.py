from recon.configs.postproc_config import PostProcConfig
from recon.configs.preproc_config import PreProcConfig


class ReconstructionConfig(object):
    """
    Full configuration (pre-proc + tool/algorithm + post-proc.
    """

    def __init__(self, functional_config, preproc_config, postproc_config):
        self.functional_config = functional_config
        self.preproc_config = preproc_config
        self.postproc_config = postproc_config

    def __str__(self):
        return str(self.preproc_config) + str(self.alg_config) + str(
            self.postproc_config)
