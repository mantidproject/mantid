# ruff: noqa: F401   # "imported but unused" error not applicable

from .textparser import TextParser

from .abinitioloader import AbInitioLoader
from .casteploader import CASTEPLoader
from .crystalloader import CRYSTALLoader
from .dmol3loader import DMOL3Loader
from .euphonicloader import EuphonicLoader
from .gaussianloader import GAUSSIANLoader
from .jsonloader import JSONLoader
from .vasploader import VASPLoader

from .tester import Tester

all_loaders = {
    "CASTEP": CASTEPLoader,
    "CRYSTAL": CRYSTALLoader,
    "DMOL3": DMOL3Loader,
    "GAUSSIAN": GAUSSIANLoader,
    "JSON": JSONLoader,
    "VASP": VASPLoader,
    "FORCECONSTANTS": EuphonicLoader,
}
