import numpy as np

TILE_SIZE = 4
EL_SHAPE_PATTERN = [
    [1, 0, 0, 0],
    [1, 0, 0, 0],
    [1, 0, 0, 0],
    [1, 1, 1, 1]
]

def apply_tile(grid, tile_type, row, col):
    if tile_type == "FULL_BLOCK":
        grid[row:row+TILE_SIZE, col:col+TILE_SIZE] = "X"
    elif tile_type == "OUTER_BOUNDARY":
        grid[row:row+TILE_SIZE, col] = "X"
        grid[row:row+TILE_SIZE, col+3] = "X"
        grid[row, col:col+TILE_SIZE] = "X"
        grid[row+3, col:col+TILE_SIZE] = "X"
    elif tile_type == "EL_SHAPE":
        for i in range(TILE_SIZE):
            for j in range(TILE_SIZE):
                if EL_SHAPE_PATTERN[i][j] == 1:
                    grid[row+i, col+j] = "X"


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


def visualize_tiling(tile_placements, grid_size):
    with open("tilesproblem.txt", 'r') as f:
        input_text = f.read()
    ll, _, _ = parse_input(input_text)

    grid = np.full((grid_size, grid_size), ".", dtype=str)  # Start with empty grid

    for tile_index, tile_type in tile_placements:
        row = (tile_index // (grid_size // TILE_SIZE)) * TILE_SIZE
        col = (tile_index % (grid_size // TILE_SIZE)) * TILE_SIZE
        apply_tile(grid, tile_type, row, col)

    counter = {
        1:0,2:0,3:0,4:0
    }
    counter2 = {
        "FULL_BLOCK" : 0, "OUTER_BOUNDARY" : 0, "EL_SHAPE" : 0
    }

    for row in range(len(ll)):
        print(" ".join(grid[row]))
        for cell in range(len(ll)):
            if grid[row][cell] == "." and ll[row][cell] > 0:
                counter[ll[row][cell]] += 1
    for _, name in tile_placements: counter2[name] += 1
    print(counter)
    print(counter2)


# grid size (20x20 landscape)
GRID_SIZE = 20
tile_placements = []
with open("tilesproblem_out.txt", 'r') as f:
    output_text = f.read()
for row in output_text.strip().split('\n'):
    num, name = row.split(" ")
    tile_placements.append((int(num), name))

visualize_tiling(tile_placements, GRID_SIZE)