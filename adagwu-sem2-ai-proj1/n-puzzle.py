import heapq


def heuristic(state, n):
    return sum(abs((val-1) // n - i) + abs((val-1) % n - j)
               for i in range(n) for j in range(n)
               if (val := state[i * n + j]))

def neighbors(state, n):
    i, j = divmod(state.index(0), n)
    moves = {(-1, 0): "Up", (1, 0): "Down", (0, -1): "Left", (0, 1): "Right"}
    for (di, dj), direction in moves.items():
        ni, nj = i + di, j + dj
        if 0 <= ni < n and 0 <= nj < n:
            new_state = list(state)
            new_state[i * n + j], new_state[ni * n + nj] = new_state[ni * n + nj], new_state[i * n + j]
            yield tuple(new_state), direction

def solve_puzzle(initial):
    n = int(len(initial) ** 0.5)
    goal = tuple(range(1, n * n)) + (0,)
    pq, visited = [(heuristic(initial, n), 0, initial, [])], set()
    while pq:
        _, cost, state, path = heapq.heappop(pq)
        if state == goal: return path
        if state in visited: continue
        visited.add(state)
        for neighbor, direction in neighbors(state, n):
            heapq.heappush(pq, (cost + 1 + heuristic(neighbor, n), cost + 1, neighbor, path + [(neighbor, direction)]))
    return "Unsolvable"

def read_input():
    with open("n-puzzle.txt", "r") as file:
        data = file.read().strip().split("\n")
        return tuple(map(int, " ".join(data).split()))

if __name__ == "__main__":
    test = 0
    initial = read_input()
    solution = solve_puzzle(initial)
    print(initial, "initial state")
    if isinstance(solution, str):
        print(solution)
    else:
        print("\n".join(map(str, solution)))
        print("Steps it took: ", len(solution))
