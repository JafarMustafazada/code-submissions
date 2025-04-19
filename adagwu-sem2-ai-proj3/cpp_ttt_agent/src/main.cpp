#include "jdevtools/curlcmd.hpp"
#include "nlohmann/json.hpp"
#include "ttt_agent/ttt_agent2.hpp"

#include <fstream>
#include <iostream>
#include <thread>

using namespace ttt_agent;
std::string apikey = "";

void play_inconsole2(playerData &p, int threadCount = 1, int n = 12, int m = 6,
		bool isalpha = true, int time = 30000, int online = 0, bool startX = false, bool dynamic = false);

void online_make_move(pii &move, std::string gameid) {
	// type=move&teamId=1447&gameId={gameid}&move={i},{j}
	curlcmd::requestData req1;
	req1.headers = {
	    "Content-Type: application/x-www-form-urlencoded",
	    "x-api-key: " + apikey,
	    "userId: 3671"};
	req1.url = "https://www.notexponential.com/aip2pgaming/api/index.php";
	req1.postData = "type=move&teamId=1447&gameId=" + gameid + "&move=" + std::to_string(move.i) + ',' + std::to_string(move.j);
	std::cout << curlcmd::sender(req1, true) << '\n';
}

pii online_read_move(std::string gameid, char opp = O) {
	// type=moves&count=1&teamId={teamId1}&gameId={gameid}
	pii move1 = {-1, -1};
	curlcmd::requestData req1;
	req1.headers = {
	    "x-api-key: " + apikey,
	    "userId: 3671"};
	req1.url = "https://www.notexponential.com/aip2pgaming/api/index.php?type=moves&count=1&teamId=1447&gameId=" + gameid;
	try {
		auto resJson = nlohmann::json::parse(curlcmd::sender(req1));
		std::cout << "Last move was made by: " << std::string(resJson["moves"][0]["symbol"]) << "\n";
		if (std::string(resJson["moves"][0]["symbol"])[0] != opp) return move1;
		move1.i = std::stoi(std::string(resJson["moves"][0]["moveX"]));
		move1.j = std::stoi(std::string(resJson["moves"][0]["moveY"]));
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	return move1;
}

// void fix_ugly_api(std::string &str1) {
// 	size_t size1 = str1.size(), count = 0;

// 	std::string str2;
// 	str2.reserve(size1);

// 	for (int i = 0; i < size1; i++) {
// 		if (str1[i] == '\\') continue;
// 		if (str1[i] == '}' && count == 3) {
// 			count++;
// 			str2.push_back('}');
// 			i++;
// 			continue;
// 		}

// 		if (str1[i] == '{') count++;
// 		if (count == 2) {
// 			str2.pop_back();
// 			count++;
// 		}
// 		str2.push_back(str1[i]);
// 	}

// 	str1 = str2;
// }

void online_read_board(playerData &p, std::string gameid, char &current) {
	// type=boardString&gameId={}
	curlcmd::requestData req1;
	req1.headers = {
	    "x-api-key: " + apikey,
	    "userId: 3671"};
	req1.url = "https://www.notexponential.com/aip2pgaming/api/index.php?type=boardMap&gameId=" + gameid;
	current = O;
	pii move;
	try {
		auto resJson = nlohmann::ordered_json::parse(curlcmd::sender(req1));
		resJson = nlohmann::ordered_json::parse(std::string(resJson["output"]));

		for (auto &elem : resJson.items()) {
			move.i = std::stoi(elem.key());
			move.j = std::stoi(elem.key().substr(elem.key().find(',') + 1));
			p.B.make(move, current);
			if (current == p.car)
				p.mylastmove = move, current = p.opp;
			else
				current = p.car;
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
}

int main(int argc, char **argv) {

	int threadCount = 0, depth = 4, n = 12, m = 6, gameid = 0, time = 28000, online = 0;
	char human = O, current = O;
	bool load = false, isalpha = true, startX = false, dynamic = false;
	std::string argument = (argc > 1) ? (argv[1]) : ("-help");

	std::cout << "total arguments: " << int((argc - 1) / 2) << "\n";
	if (argument == "-help") {
		std::cout << "\nneed at least one argument to start like: -depth 3\n";
		std::cout << "-n {board size. default(12)}\n";
		std::cout << "-m {win line length. default(6)}\n\n";
		std::cout << "-threads {how many thread to use, where 0 - no threading, 1 - maximum threading, 2 - 2 threads and etc. default(0)}\n";
		std::cout << "-depth {depth of minimax. default(4)}\n\n";
		std::cout << "-player {your/oponent symbol against ai, can be X or O where O start first. default(O)}\n";
		std::cout << "-alpha {should it use alpha-beta purning. default(1)}\n";
		std::cout << "-time {time limit for each step in milli seconds. default(30000)}\n";
		std::cout << "-start {1 - start with X instead, useful with -load. default(0)}\n";
		std::cout << "-dynamic {1 - change depth depending on time limit. default(0)}\n";
		std::cout << "-online {gameId - for ai making auto request. Disables player input (reads from api). default(0)}\n";
		std::cout << "-load {should it load game board from map.txt. default(0)}\n\n";
		std::cout << "Here is example of map.txt:\n";
		std::cout << "XX----------\n";
		std::cout << "------------\n";
		std::cout << "--O---------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "------------\n";
		std::cout << "O----------O\n";
		return 0;
	}

	argc--;
	for (int i = 1; i < argc; i += 2) {
		std::string argument = argv[i];
		std::cout << argument << " " << argv[i + 1] << "\n";
		if (argument == "-load") load = std::stoi(argv[i + 1]);
		else if (argument == "-n") n = std::stoi(argv[i + 1]);
		else if (argument == "-m") m = std::stoi(argv[i + 1]);
		else if (argument == "-threads") threadCount = std::stoi(argv[i + 1]);
		else if (argument == "-online") online = std::stoi(argv[i + 1]);
		else if (argument == "-time") time = std::stoi(argv[i + 1]);
		else if (argument == "-alpha") isalpha = std::stoi(argv[i + 1]);
		else if (argument == "-depth") depth = std::stoi(argv[i + 1]);
		else if (argument == "-start") startX = std::stoi(argv[i + 1]);
		else if (argument == "-dynamic") dynamic = std::stoi(argv[i + 1]);
		else if (argument == "-player") human = argv[i + 1][0];
		else {
			std::cout << "Error with param:{" << argument << "}\n";
			return -1;
		}
	}

	if (m > n) m = n;
	if (human != X && human != O) human = X;
	if (threadCount == 1) threadCount = std::thread::hardware_concurrency() - 1;

	playerData p1{Board(n, m)};
	p1.opp = human;
	p1.car = human == X ? O : X;
	p1.depth = depth;

	if (load) {
		std::cout << "reading..\n";
		std::ifstream file("map.txt");
		try {
			char read1 = '\n';
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					if (read1 == '\n') read1 = file.get();
					if (read1 == X) p1.B.make({i, j}, X);
					else if (read1 == O) p1.B.make({i, j}, O);
					file.get(read1);
				}
			}
			file.close();
		} catch (const std::exception &) {
			file.close();
		}
	}
	if (online) {
		try {
			std::ifstream file("apikey.txt");
			std::getline(file, apikey);
		} catch (const std::exception &e) {
			std::cerr << e.what() << '\n';
		}

		online_read_board(p1, std::to_string(online), current);
	}
	if (current == X) startX = true;

	play_inconsole2(p1, threadCount, n, m, isalpha, time, online, startX, dynamic);

	return 0;
}

void play_inconsole2(playerData &p, int threadCount, int n, int m,
		bool isalpha, int time, int online, bool startX, bool dynamic) {
	std::cout << "\n AI(" << p.car << "); depth: " << p.depth << "; win length: " << m << ";";
	if (isalpha) std::cout << " +alpha-beta;";
	if (threadCount) std::cout << " threads: " << threadCount << ";";
	std::cout << "\n";

	std::string input, gameid = std::to_string(online);
	char current = startX ? X : O;
	bool istie = false;
	int localdepth = p.depth;
	pii move;

	playerData p2{p.B};
	p2.car = p.opp;
	p2.opp = p.car;
	p2.depth = p.depth;

	do {
		std::cout << p.B.display();

		if (online && current == p.opp) {
			// std::cout << "Send move?";
			pii oppmove = {-1, -1};
			while (oppmove.i == -1) {
				std::cout << "Read move? (just enter): ";
				std::cin >> input;
				oppmove = online_read_move(gameid, p.opp);
			}
			move = oppmove;
		} else if (current == p.opp) {
			// do {
			// 	std::cout << "Player " << current << ", enter your move (row col, index starts from 0): ";
			// 	std::cin >> move.i >> move.j;
			// } while (move.i < 0 || move.i >= n || move.j < 0 || move.j >= n || p.B.g[move.i][move.j] != E);
			std::cout << "AI2 (" << current << ")'s turn...\n";
			if (threadCount)
				move = minimaxBestThreading(p2, isalpha, threadCount, time);
			else
				move = minimaxBest(p2, isalpha, time);
		} else {
			std::cout << "AI (" << current << ")'s turn...\n";
			if (threadCount)
				move = minimaxBestThreading(p, isalpha, threadCount, time);
			else
				move = minimaxBest(p, isalpha, time);
			if (online) online_make_move(move, gameid);
		}

		std::cout << move.i << " : " << move.j << " (depth: " << p.depth << ")\n";
		if (!dynamic) p.depth = localdepth;
		p.B.make(move, current);
		current = (current == X ? O : X);

		if (!dynamic) p2.depth = localdepth;
		p2.B.make(move, current);

		if (p.B.full()) istie = true;
	} while (!istie && !p.B.isMoveWin(move));

	if (p.B.isMoveWin(move)) {
		std::cout << p.B.g[move.i][move.j] << " is winner\n";
	}
	for (auto &row : p.B.g) {
		for (auto c : row)
			std::cout << c << " ";
		std::cout << "\n";
	}
	std::cout << '\n';
}