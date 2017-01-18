from recon.configs.functional_config import FunctionalConfig
from recon.configs.preproc_config import PreProcConfig
from recon.configs.postproc_config import PostProcConfig


class ReconstructionConfig(object):
    """
    Full configuration (pre-proc + tool/algorithm + post-proc.
    """

    def __init__(self, functional_config, preproc_config, postproc_config):
        # just some sanity checks
        if not isinstance(functional_config, FunctionalConfig):
            raise ValueError(
                "Functional config is invalid type. The script might be corrupted.")

        if not isinstance(preproc_config, PreProcConfig):
            raise ValueError(
                "Functional config is invalid type. The script might be corrupted.")

        if not isinstance(postproc_config, PostProcConfig):
            raise ValueError(
                "Functional config is invalid type. The script might be corrupted.")

        self.func = functional_config
        self.pre = preproc_config
        self.post = postproc_config
        self.helper = None

    def __str__(self):
        return str(self.func) + str(self.pre) + str(self.post)

    @staticmethod
    def empty_init():
        """
        Create and return a ReconstructionConfig with all the default values.

        This function is provided here to create a config with the defaults, 
        but not go through the hassle of importing every single config and 
        then constructing it manually. 
        This method does that for you!
        """
        return ReconstructionConfig(FunctionalConfig(), PreProcConfig(), PostProcConfig())
