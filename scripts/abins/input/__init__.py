# flake8: noqa F401   # "imported but unused" error not applicable


from .casteploader import CASTEPLoader
from .crystalloader import CRYSTALLoader
from .dmol3loader import DMOL3Loader
from .euphonicloader import EuphonicLoader
from .gaussianloader import GAUSSIANLoader
from .vasploader import VASPLoader


all_loaders = {
    "CASTEP": CASTEPLoader,
    "CRYSTAL": CRYSTALLoader,
    "DMOL3": DMOL3Loader,
    "GAUSSIAN": GAUSSIANLoader,
    "VASP": VASPLoader,
    "FORCECONSTANTS": EuphonicLoader,
}
