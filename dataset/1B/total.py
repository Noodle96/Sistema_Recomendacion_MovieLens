from pathlib import Path
from typing import List

import numpy as np


def count_rows_npz(file_path: Path) -> int:
    """
    Cuenta cuántas filas (interacciones) hay en un archivo .npz.
    """
    data = np.load(file_path, allow_pickle=False)
    arr = data["arr_0"]
    return arr.shape[0]


def main() -> None:
    dataset_dir: Path = Path("ml-20mx16x32")

    train_files: List[Path] = sorted(dataset_dir.glob("trainx16x32_*.npz"))
    test_files: List[Path] = sorted(dataset_dir.glob("testx16x32_*.npz"))

    total_train: int = 0
    total_test: int = 0

    print("Contando TRAIN...")
    for file in train_files:
        rows = count_rows_npz(file)
        print(f"{file.name}: {rows}")
        total_train += rows

    print("\nContando TEST...")
    for file in test_files:
        rows = count_rows_npz(file)
        print(f"{file.name}: {rows}")
        total_test += rows

    total: int = total_train + total_test

    print("\n==============================")
    print(f"Total TRAIN: {total_train}")
    print(f"Total TEST: {total_test}")
    print(f"TOTAL GENERAL: {total}")
    print("==============================")


if __name__ == "__main__":
    main()
