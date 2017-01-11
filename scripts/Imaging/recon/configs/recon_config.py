from recon.configs.postproc_config import PostProcConfig
from recon.configs.preproc_config import PreProcConfig


class ReconstructionConfig(object):
    """
    Full configuration (pre-proc + tool/algorithm + post-proc.
    """

    def __init__(self, functional_config, preproc_config, postproc_config):
        self.func = functional_config
        self.pre = preproc_config
        self.post = postproc_config

    def __str__(self):
        return str(self.func) + str(self.pre) + str(self.post)
