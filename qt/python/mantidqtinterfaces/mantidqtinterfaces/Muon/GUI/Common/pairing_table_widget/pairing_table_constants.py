# pair_table_constants.py
from typing import List

inverse_pair_columns = {"pair_name": 0, "to_analyse": 1, "group_1": 2, "group_2": 3, "alpha": 4, "guess_alpha": 5}


def get_index_of_text(selector, text):
    for i in range(selector.count()):
        if str(selector.itemText(i)) == text:
            return i
    return 0


pair_columns = {0: "pair_name", 1: "to_analyse", 2: "group_1", 3: "group_2", 4: "alpha", 5: "guess_alpha"}


def get_pair_columns() -> List[str]:
    return list(pair_columns.values())
