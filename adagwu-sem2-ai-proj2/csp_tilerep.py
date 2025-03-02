TILE_SIZE = 4


def parse_input(input_text : str):
    # sections: # Landscape, # Tiles, # Targets.
    sections = input_text.split("# ") 
    landscape_data = sections[1].split("\n")[1:]
    size = len(sections[1].strip().split("\n")) - 1
    print(size,"x",size, sep="")

    landscape = []
    for j in range(size):
        row = landscape_data[j]
        landscape_row = []
        for i in range(size):
            if row[i * 2] == " ": landscape_row.append(0)
            else: landscape_row.append(int(row[i * 2]))
        landscape.append(landscape_row)
            
    tiles_line = sections[2].strip().split("\n")[1].strip().strip("{}")
    tiles_dict = {}
    for part in tiles_line.split(","):
        key, val = part.split("=")
        tiles_dict[key.strip()] = int(val.strip())
    
    targets = {}
    targets_data = sections[3].strip().split("\n")[1:]
    for target in targets_data:
        color, count = target.split(":")
        targets[int(color)] = int(count)
    return landscape, tiles_dict, targets


def get_patch(landscape, tile_row, tile_col):
    sr, sc = tile_row * TILE_SIZE, tile_col * TILE_SIZE
    return [landscape[sr + i][sc:sc + TILE_SIZE] for i in range(TILE_SIZE)]


def compute_contribution(patch, tile_type):
    contrib = {}
    if tile_type == 'FULL_BLOCK': return contrib
    elif tile_type == 'OUTER_BOUNDARY':
        for i in [1, 2]:
            for j in [1, 2]:
                c = patch[i][j]
                if c != 0: contrib[c] = contrib.get(c, 0) + 1
        return contrib
    elif tile_type == 'EL_SHAPE':
        pattern = [
            [1, 0, 0, 0],
            [1, 0, 0, 0],
            [1, 0, 0, 0],
            [1, 1, 1, 1]
        ]
        for i in range(TILE_SIZE):
            for j in range(TILE_SIZE):
                if pattern[i][j] == 0:
                    c = patch[i][j]
                    if c != 0: contrib[c] = contrib.get(c, 0) + 1
        return contrib


def ac3(domains, assignment, tile_contrib, targets, num_tiles):
    unassigned = [v for v in range(num_tiles) if v not in assignment]
    current = {1: 0, 2: 0, 3: 0, 4: 0}

    for v, ttype in assignment.items():
        for color, cnt in tile_contrib[v][ttype].items():
            if color == 0: continue
            current[color] += cnt

    # For each unassigned tile, over its domain, min/max per color.
    min_add = {1: 0, 2: 0, 3: 0, 4: 0}
    max_add = {1: 0, 2: 0, 3: 0, 4: 0}

    for v in unassigned:
        mins = {1: float('inf'), 2: float('inf'), 3: float('inf'), 4: float('inf')}
        maxs = {1: float('-inf'), 2: float('-inf'), 3: float('-inf'), 4: float('-inf')}

        for val in domains[v]:
            contrib = tile_contrib[v][val]

            for c in [1, 2, 3, 4]:
                mins[c] = min(mins[c], contrib.get(c, 0))
                maxs[c] = max(maxs[c], contrib.get(c, 0))

        for c in [1, 2, 3, 4]:
            min_add[c] += mins[c] if mins[c] != float('inf') else 0
            max_add[c] += maxs[c] if maxs[c] != float('-inf') else 0

    for c in [1, 2, 3, 4]:
        total_min = current[c] + min_add[c]
        total_max = current[c] + max_add[c]
        if targets[c] < total_min or targets[c] > total_max: return False
    return True

# Backtracking with MRV and LCV
def select_variable(assignment, domains):
    unassigned = [v for v in domains if v not in assignment]
    return min(unassigned, key=lambda v: len(domains[v]))

def order_values(var, domains, tile_contrib):
    return sorted(domains[var], key=lambda val: sum(tile_contrib[var][val].values()))

def global_target_ok(assignment, tile_contrib, targets):
    total = {1: 0, 2: 0, 3: 0, 4: 0}
    for v, ttype in assignment.items():
        for c, cnt in tile_contrib[v][ttype].items():
            total[c] += cnt
    return all(total[c] == targets[c] for c in targets)

def backtrack(assignment, domains, tile_contrib, remaining_tiles, targets, num_tiles):
    if len(assignment) == num_tiles:
        return assignment if global_target_ok(assignment, tile_contrib, targets) else None
    
    var = select_variable(assignment, domains)
    for val in order_values(var, domains, tile_contrib):
        if remaining_tiles[val] <= 0: continue
        new_assignment = assignment.copy()
        new_assignment[var] = val
        new_remaining = remaining_tiles.copy()
        new_remaining[val] -= 1
        new_domains = {v: list(vals) for v, vals in domains.items()}
        new_domains[var] = [val]

        if ac3(new_domains, new_assignment, tile_contrib, targets, num_tiles):
            result = backtrack(new_assignment, new_domains, tile_contrib, new_remaining, targets, num_tiles)
            if result is not None: return result
    return None

def solve_csp(landscape, tiles_dict, targets):
    n = len(landscape)
    tile_rows = n // TILE_SIZE
    tile_cols = n // TILE_SIZE
    num_tiles = tile_rows * tile_cols
    tile_contrib = {}
    for idx in range(num_tiles):
        r, c = divmod(idx, tile_cols)
        patch = get_patch(landscape, r, c)
        tile_contrib[idx] = {
            'FULL_BLOCK': compute_contribution(patch, 'FULL_BLOCK'),
            'OUTER_BOUNDARY': compute_contribution(patch, 'OUTER_BOUNDARY'),
            'EL_SHAPE': compute_contribution(patch, 'EL_SHAPE')
        }
    domains = {v: ['FULL_BLOCK', 'OUTER_BOUNDARY', 'EL_SHAPE'] for v in range(num_tiles)}
    remaining_tiles = tiles_dict.copy()
    sol = backtrack({}, domains, tile_contrib, remaining_tiles, targets, num_tiles)
    return sol


if __name__ == "__main__":
    with open("tilesproblem.txt", "r") as f:
        text = f.read()
    landscape, tiles_dict, targets = parse_input(text)
    solution = solve_csp(landscape, tiles_dict, targets)
    if solution is None: print("No solution found")
    else:
        for idx in sorted(solution):
            print(f"{idx} {solution[idx]}")
