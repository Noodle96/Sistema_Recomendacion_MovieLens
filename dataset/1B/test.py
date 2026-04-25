from pathlib import Path
from typing import Any

import numpy as np


def inspect_npz_file(file_path: Path) -> None:
    """
    Inspecciona un archivo .npz e imprime sus claves, shapes y tipos.
    """
    data: Any = np.load(file_path, allow_pickle=False)

    print(f"Archivo: {file_path.name}")
    print(f"Tipo del objeto cargado: {type(data)}")
    print(f"Claves dentro del npz: {data.files}")
    print("-" * 60)

    for key in data.files:
        array: np.ndarray = data[key]
        print(f"Clave: {key}")
        print(f"  shape: {array.shape}")
        print(f"  dtype: {array.dtype}")

        if array.ndim == 0:
            print(f"  valor: {array}")
        else:
            print(f"  primeros elementos: {array[:10]}")
        print("-" * 60)


def main() -> None:
    """
    Punto de entrada principal.
    """
    dataset_dir: Path = Path("ml-20mx16x32")
    # file_path: Path = dataset_dir / "testx16x32_15.npz"
    file_path: Path = dataset_dir / "trainx16x32_4.npz"

    inspect_npz_file(file_path)


if __name__ == "__main__":
    main()
