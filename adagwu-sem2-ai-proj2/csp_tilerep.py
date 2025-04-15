TILE_SIZE = 4
def parse_input(input_text : str):
	# sections: # Landscape, # Tiles, # Targets.
	sections = input_text.split("# ") 
	landscape_data = sections[1].split("\n")[1:]
	print(landscape_data)
	size = len(sections[1].strip().split("\n")) - 1
	print(size,"x",size, sep="")
	landscape = []
	
	for j in range(size):
		row = landscape_data[j]
		landscape_row = []
		for i in range(size):
			if i * 2 > len(row): landscape_row.append(0)
			elif row[i * 2] == " ": landscape_row.append(0)
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

def rotate_90(pattern):
	return [list(row) for row in zip(*pattern[::-1])]

def get_el_shapes():
	base = [
		[1, 0, 0, 0],
		[1, 0, 0, 0],
		[1, 0, 0, 0],
		[1, 1, 1, 1]
	]
	return [base, rotate_90(base), rotate_90(rotate_90(base)), rotate_90(rotate_90(rotate_90(base)))]

EL_SHAPES = get_el_shapes()

def compute_contribution(patch, tile_type, rotation=0):
	contrib = {}
	if tile_type == 'FULL_BLOCK': return contrib
	elif tile_type == 'OUTER_BOUNDARY':
		for i in [1, 2]:
			for j in [1, 2]:
				c = patch[i][j]
				if c != 0: contrib[c] = contrib.get(c, 0) + 1
		return contrib
	elif tile_type == 'EL_SHAPE':
		shape = EL_SHAPES[rotation]
		for i in range(TILE_SIZE):
			for j in range(TILE_SIZE):
				if shape[i][j] == 0:
					c = patch[i][j]
					if c != 0: contrib[c] = contrib.get(c, 0) + 1
		return contrib

def ac3(domains, assignment, tile_contrib, targets, num_tiles):
	unassigned = [v for v in range(num_tiles) if v not in assignment]
	current = {1: 0, 2: 0, 3: 0, 4: 0}
	
	for v, ttype in assignment.items():
		tile_type, rotation = ttype if isinstance(ttype, tuple) else (ttype, 0)
		for color, cnt in tile_contrib[v][tile_type][rotation].items():
			if color == 0: continue
			current[color] += cnt
			
	min_add = {1: 0, 2: 0, 3: 0, 4: 0}
	max_add = {1: 0, 2: 0, 3: 0, 4: 0}

	for v in unassigned:
		mins = {1: float('inf'), 2: float('inf'), 3: float('inf'), 4: float('inf')}
		maxs = {1: float('-inf'), 2: float('-inf'), 3: float('-inf'), 4: float('-inf')}

		for val in domains[v]:
			tile_type, rotation = val if isinstance(val, tuple) else (val, 0)
			contrib = tile_contrib[v][tile_type][rotation]
			
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

def select_variable(assignment, domains):
	unassigned = [v for v in domains if v not in assignment]
	return min(unassigned, key=lambda v: len(domains[v]))

def order_values(var, domains, tile_contrib):
	return sorted(domains[var], key=lambda val: sum(tile_contrib[var][val[0]][val[1]].values()))

def global_target_ok(assignment, tile_contrib, targets):
	total = {1: 0, 2: 0, 3: 0, 4: 0}
	for v, ttype in assignment.items():
		tile_type, rotation = ttype if isinstance(ttype, tuple) else (ttype, 0)
		for c, cnt in tile_contrib[v][tile_type][rotation].items(): total[c] += cnt
	return all(total[c] == targets[c] for c in targets)

def backtrack(assignment, domains, tile_contrib, remaining_tiles, targets, num_tiles):
	if len(assignment) == num_tiles:
		return assignment if global_target_ok(assignment, tile_contrib, targets) else None

	var = select_variable(assignment, domains)
	for val in order_values(var, domains, tile_contrib):
		tile_type, _ = val
		if remaining_tiles[tile_type] <= 0: continue
		new_assignment = assignment.copy()
		new_assignment[var] = val
		new_remaining = remaining_tiles.copy()
		new_remaining[tile_type] -= 1
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
			'FULL_BLOCK': {0: compute_contribution(patch, 'FULL_BLOCK')},
			'OUTER_BOUNDARY': {0: compute_contribution(patch, 'OUTER_BOUNDARY')},
			'EL_SHAPE': {rot: compute_contribution(patch, 'EL_SHAPE', rot) for rot in range(4)}
		}

	domains = {v: [('FULL_BLOCK', 0), ('OUTER_BOUNDARY', 0)] + [('EL_SHAPE', rot) for rot in range(4)] for v in range(num_tiles)}
	remaining_tiles = tiles_dict.copy()
	return backtrack({}, domains, tile_contrib, remaining_tiles, targets, num_tiles)

if __name__ == "__main__":
	with open("tilesproblem.txt", "r") as f:
		text = f.read()
	landscape, tiles_dict, targets = parse_input(text)
	solution = solve_csp(landscape, tiles_dict, targets)
	if solution is None: print("No solution found")
	else:
		for idx in sorted(solution):
			print(f"{idx} {solution[idx]}")