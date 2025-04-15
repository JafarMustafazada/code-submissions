import requests
import json

api_key = "key"
user_id = "3671"
teamId1 = "1447"


headers01 = {
	'userId': user_id,
	'x-api-key': api_key,
	'User-Agent': 'curl/8.10.1',
	'Content-Type': 'application/x-www-form-urlencoded'
}
def sends(payload, is_get = False, headers1 = headers01):
	url = "http://www.notexponential.com/aip2pgaming/api/index.php"
	method = "POST" 
	if is_get:
		method = "GET"
		url += "?" + payload
		payload = {}
	response = requests.request(method, url, headers=headers1, data=payload)
	if response.status_code == 200: return response
	else: print(f"Error: {response.status_code} - {response.text}")

def create_game(team2, start_second = False, boardSize = 12, target = 6):
	team1 = teamId1
	if start_second:
		team1 = team2
		team2 = teamId1
	payload1 = f"type=game&teamId1={team1}&teamId2={team2}&gameType=TTT&boardSize={boardSize}&target={target}"
	return sends(payload1)

def make_move(gameid, i, j):
	payload1 = f"type=move&gameId={gameid}&teamId={teamId1}&move={i},{j}"
	return sends(payload1)

## getters ##
def get_games(isOpen = False):
	typeis = "myGames"
	if (isOpen): typeis = "myOpenGames"
	payload1 = f"type={typeis}"
	print(sends(payload1, True).text)

def get_moves(gameid):
	payload1 = f"type=moves&gameId={gameid}&teamId={teamId1}&count=1"
	print(sends(payload1, True).text)
	
def get_details(gameid):
	payload1 = f"type=gameDetails&gameId={gameid}"
	print(sends(payload1, True).text)

def get_s_board(gameid):
	payload1 = f"type=boardString&gameId={gameid}"
	data1 = json.loads(sends(payload1, True).text)
	sboard = str(data1["output"])
	for line in sboard.split('\\n'):
		print(line)
	print()

def get_m_board(gameid):
	payload1 = f"type=boardMap&gameId={gameid}"
	data1 = json.loads(sends(payload1, True).text)
	mboard = json.loads(data1["output"])
	for key, value in mboard.items():
		print(f"{key}: {value}")



## not going to work :< ##
# def farm_points():
# 	while(True):
# 		print("(0 for breakin loop): ", end="")
# 		if (input() == "0"): break
		
# 		obj_create = json.loads(create_game("1447").text)
# 		if (obj_create["code"] == "FAIL"):
# 			print(obj_create["message"])
# 			continue

# 		gameid = obj_create["gameId"]
# 		movei = 0, movej = 0
# 		while(True):
# 			obj_move = json.loads(make_move(gameid, movei, movej).text)
# 			if (obj_move["code"] == "FAIL"):
# 				print(obj_create["message"])
# 				print("should we continue? (0 for breakin loop): ", end="")
# 				if (input() == "0"): break
# 				continue
# 			movei += 1
# 			movej += 1

		

## main ##
print("GG")
# gameid = 5191
# gameid = 5186
# print(create_game("1447").text)
gameid = 5207
# get_s_board(gameid)
# print(make_move(gameid, 3, 3).text)
get_s_board(gameid)

# get_games()
# get_moves(gameid)
# get_m_board(gameid)
# get_details(gameid)
# get_games(True)


# print(make_move(gameid, 3, 5).text)
# get_m_board(gameid)

