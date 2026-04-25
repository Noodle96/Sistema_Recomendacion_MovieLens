import time

def solve2():
    vectorA: list[int] = list(range(10000))
    vectorB: list[int] = list(range(10000))
    vectorC: list[int] = [0] * 10000

    start_time: float = time.time()

    for e in range(len(vectorA)):
        for j in range(len(vectorB)):
            vectorC[e] = vectorA[e] + vectorB[j]

    end_time: float = time.time()
    elapsed_time: float = end_time - start_time
    print(f"Time taken: {elapsed_time:.6f} seconds")

# Llamada a la función
solve2()
