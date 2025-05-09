#include "jdevtools/jdevcurl.hpp"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>

using json = nlohmann::json;
using namespace jdevtools;
using namespace std;


static constexpr int A = 4;
static constexpr int GRID_SIZE = 40;
static constexpr int N = 0, E = 1, S = 2, W = 3; // Direction indices

const vector<char> DIRECTIONS = {'N', 'E', 'S', 'W'};
const vector<char> DIRECTIONS2 = {'^', '>', 'v', '<'};


inline static int TIME_DELAY = 6;
inline static int VISUAL_MODE = 0;


// helpers
int json_get_int(json &js, string key) {
	if (js[key].is_number()) return js[key].get<int>();
	return stoi(js[key].get<string>());
}
int action_fromchar(char move) {
	if (move == 'N') return N;
	if (move == 'E') return E;
	if (move == 'S') return S;
	if (move == 'W') return W;
	else return -1;
}


struct Cell {
	int epsilon = (1 << A);
	vector<double> direction = { 0.0, 0.0, 0.0, 0.0 };
};


void to_json(nlohmann::json& j, const Cell& c) {
	j["epsilon"] = c.epsilon;
	j["direction"] = c.direction;
}

void from_json(const nlohmann::json& j, Cell& c) {
	c.epsilon = j["epsilon"];
	c.direction = j["direction"].get<vector<double> >();
}



// API
class GridAPI {
	public:
		inline static int userid1 = 3671;
		inline static int teamid1 = 1447;
		inline static int worldid1 = 1;
		inline static vector<string> haeders;
	
		static void readyH() {
			string apikey = "";
			{
				ifstream file("apikey.txt");
				if (!file) {
					cout << "\nno apikey.\n";
					return;
				}
				getline(file, apikey);
			}
			haeders = {
				"Content-Type: application/x-www-form-urlencoded",
				"x-api-key: " + apikey,
				"userId: " + to_string(userid1)
			};
		}
	
		static pair<pair<int, int>, double> makeMove(char direction) {
			requestData req;
			req.headers = haeders;
			req.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php";
			req.postData = "type=move&teamId=" + to_string(teamid1) + "&worldId=" + to_string(worldid1) + "&move=" + direction;
	
			string str = sender(req, (req.postData.size()));
			cout << str;
			json js = json::parse(str);
	
			if (!js.contains("reward")) {
				cout << "\nno reward\n";
				return {{-1, -1}, 0.0};
			}
	
			double reward = js["reward"];
			int r = -1, c = -1;
	
			try {
				if (js["newState"]["x"].is_number())
					r = js["newState"]["x"];
				else
					r = stoi(js["newState"]["x"].get<string>());
		
				if (js["newState"]["y"].is_number())
					c = js["newState"]["y"];
				else
					c = stoi(js["newState"]["y"].get<string>());
			}
			catch(const std::exception& e) {
				r = -1, c = -1;
			}
	
			return {{r, c}, reward};
		}
	
		static pair<int, int> enterWorld() {
			requestData en;
			en.headers = haeders;
			en.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php";
			en.postData = "type=enter&worldId=" + to_string(worldid1) + "&teamId=" + to_string(teamid1);
	
			string str = sender(en, true);
			json js = json::parse(str);
			int world = -1;
			cout << str << '\n';
	
			if (js["worldId"].is_number()) world = js["worldId"];
			else world = stoi(js["worldId"].get<string>());
	
			if (world == worldid1) return {0, 0};
			cout << "\n error. current world is " << world << " while requested world is " << worldid1 << '\n';
			return {-1, -1};
		}
	
		static pair<int, int> getInitialPosition() {
			requestData req;
			req.headers = haeders;
			req.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php?type=location&teamId=" + to_string(teamid1);
	
			string str = sender(req, (req.postData.size()));
			cout << str << '\n';
			json js = json::parse(str);
	
			int world = -1, r = -1, c = -1;
	
			if (js["world"].is_number())
				world = js["world"];
			else
				world = stoi(js["world"].get<string>());
	
			if (world == -1 && VISUAL_MODE != 2) return enterWorld();
	
			str = js["state"].get<string>();
			size_t p1 = str.find(':');
			r = stoi(str.substr(0, p1));
			c = stoi(str.substr(p1 + 1));
	
			return {r, c};
		}
	};
	
class QLearningSolver {
private:
	// Q-table: maps state-action pairs to expected rewards
	unordered_map<string, Cell> qTable;

	// Current position
	pair<int, int> currentPos;

	// Learning parameters
	double alpha = 0.1;          // Learning rate
	double gamma = 0.99;         // Discount factor
	double epsilonDecay = 0.5;   // Decay rate for epsilon
	//double minEpsilon = 0.01;    // Minimum exploration rate
	const int minEpsilon = 1;

	// Statistics
	int episodeCount = 0;
	int TARGET_X = -1, TARGET_Y = -1;
	bool targetFounded = false;

	// Random number generator
	random_device rd;
	mt19937 rng;

	// Convert position to state string
	string stateToString(const pair<int, int> &pos) {
		return to_string(pos.first) + ":" + to_string(pos.second);
	}

	// Choose action using epsilon-greedy policy
	int chooseAction(const string &state) {
		uniform_int_distribution<int> dist(0, (1 << A));

		// Ensure state exists in Q-table
		if (qTable.find(state) == qTable.end()) {
			qTable[state].direction = vector<double>(4, 0.0);
			qTable[state].epsilon = (1 << A);
		}

		// Epsilon-greedy action selection
		if (dist(rng) < qTable[state].epsilon) {
			// Exploration: choose random action
			uniform_int_distribution<int> actionDist(0, 3);
			return actionDist(rng);
		} else {
			// Exploitation: choose best action
			auto &qValues = qTable[state].direction;
			return max_element(qValues.begin(), qValues.end()) - qValues.begin();
		}
	}

	// Update Q-value for a state-action pair
	void updateQValue(const string &state, int action, double reward, const string &nextState) {
		// Ensure states exist in Q-table
		if (qTable.find(state) == qTable.end()) {
			qTable[state].direction = vector<double>(4, 0.0);
			qTable[state].epsilon = (1 << A);
		}
		if (qTable.find(nextState) == qTable.end()) {
			qTable[nextState].direction = vector<double>(4, 0.0);
			qTable[nextState].epsilon = (1 << A);
		}

		// Get maximum Q-value for next state
		double maxNextQ = *max_element(qTable[nextState].direction.begin(), qTable[nextState].direction.end());

		// Q-learning update rule
		qTable[state].direction[action] = (1 - alpha) * qTable[state].direction[action] 
		+ alpha * (reward + gamma * maxNextQ);

		qTable[state].epsilon = max(minEpsilon, (qTable[state].epsilon >> 1));
	}

	// Decay epsilon for exploration rate
	// void decayEpsilon() {
	// 	epsilon = max(minEpsilon, epsilon * epsilonDecay);
	// }
	
public:
	// Load Q-table from file
	void loadQTable() {
		{
			ifstream file(("world_" + to_string(GridAPI::worldid1) + "_tabv2.json"));
			if (!file.is_open()) {
				cout << "No existing Q-table found, starting fresh." << endl;
				return;
			}
			json js;
			file >> js;
			TARGET_X = js["TARGET_X"].get<int>();
			TARGET_Y = js["TARGET_Y"].get<int>();
			qTable = js["Q"].get<unordered_map<string, Cell> >();
			episodeCount = js["trained"];
			targetFounded = js["founded"];
		}
	}

	// Save Q-table to file
	void saveQTable() {
		ofstream file(("world_" + to_string(GridAPI::worldid1) + "_tabv2.json"));
		if (!file.is_open()) {
			cerr << "Cannot open file for saving Q-table." << endl;
			return;
		}
		json js;
		js["Q"] = qTable;
		js["trained"] = episodeCount;
		js["founded"] = targetFounded;
		js["TARGET_X"] = TARGET_X;
		js["TARGET_Y"] = TARGET_Y;
		file << js.dump(2);
		file.close();
	}

	QLearningSolver() : rng(rd()) {
		// loadQTable(); // Attempt to load existing Q-table
	}

	~QLearningSolver() {
		// Save Q-table when done
		saveQTable();
	}

	// Run a training episode
	void runEpisode(int maxSteps = 1000) {
		int steps = 0;

		// Reset to initial position for this episode
		currentPos = GridAPI::getInitialPosition();
		string state = stateToString(currentPos);

		while (steps < maxSteps) {
			auto wait_until = std::chrono::system_clock::now() + std::chrono::seconds(TIME_DELAY);
			steps++;

			// Choose action using epsilon-greedy
			int action = chooseAction(state);
			char direction = DIRECTIONS[action];

			// Take action and observe result
			auto [newPos, reward] = GridAPI::makeMove(direction);
			cout << " " << DIRECTIONS2[action];
			string nextState = stateToString(newPos);

			// Check if target found
			if (reward >= 1000) {
				targetFounded = true;
				TARGET_X = currentPos.first;
				TARGET_Y = currentPos.second;
				cout << "\nTarget found in episode " << episodeCount
						<< " after " << steps << " steps!" << endl;
				break;
			}

			// Update Q-value
			updateQValue(state, action, reward, nextState);
			saveQTable();

			if (steps % 100 == 0) {
				cout << "Steps taken: " << steps << endl;
			}
			// Update current state and position
			state = nextState;
			currentPos = newPos;
			
			cout << "\nasleep..";
			std::this_thread::sleep_until(wait_until);
			cout << "awake.. ";
		}

		// Decay exploration rate
		episodeCount++;
		saveQTable();

		if (!targetFounded) {
			cout << "Episode " << episodeCount << " completed with "
					<< steps << " steps. No target found." << endl;
		}
	}

	// Manually modifies Q-table to force direction toward target like (38,36)
	void modifyQTableForTarget(bool clean = false) {
		std::cout << "Modifying Q-table to pursue target at (" << TARGET_X << "," << TARGET_Y << ")..." << std::endl;
		// Clear any existing Q-table to start fresh
		qTable.clear();
		
		// For each cell in the grid
		for (int x = 0; x < GRID_SIZE; x++) {
			for (int y = 0; y < GRID_SIZE; y++) {
				std::string state = std::to_string(x) + "," + std::to_string(y);
				std::vector<double> qValues(4, 0.0);
				
				// Calculate direction to target
				int dx = TARGET_X - x;
				int dy = TARGET_Y - y;
				int distance = std::abs(dx) + std::abs(dy);
				
				// Value is higher for being closer to target
				double baseValue = 1000 - distance * 10;
				
				// Set Q-values based on direction to target
				if (dx < 0) qValues[E] += baseValue; // Need to go East
				if (dx > 0) qValues[W] += baseValue; // Need to go West
				if (dy > 0) qValues[S] += baseValue; // Need to go South
				if (dy < 0) qValues[N] += baseValue; // Need to go North
				
				// At the target itself, all directions have high value
				if (x == TARGET_X && y == TARGET_Y) {
					qValues = {2000, 2000, 2000, 2000};
				}
				
				if (clean) qTable[state].direction = qValues;
				else {
					for (int i = 0; i < A; i++) {
						qTable[state].direction[i] += qValues[i];
					}
				}
				qTable[state].epsilon = minEpsilon;
			}
		}
		
		std::cout << "Q-table modified to prioritize target at (" << TARGET_X << "," << TARGET_Y << ")" << std::endl;
	}
	

	// Find the optimal path to the target using learned Q-values
	void findOptimalPath(int maxSteps = 1000) {
		if (!targetFounded) cout << "\nTarget yet to be founded" << endl;
		cout << "Finding optimal path to target..." << endl;

		// Reset to initial position
		currentPos = GridAPI::getInitialPosition();
		string state = stateToString(currentPos);

		int steps = 0;
		bool foundTarget = false;

		// Follow the policy greedily (no exploration)
		while (steps < maxSteps) {
			auto wait_until = std::chrono::system_clock::now() + std::chrono::seconds(TIME_DELAY);
			steps++;

			// Ensure state exists in Q-table
			if (qTable.find(state) == qTable.end()) {
				cout << "Warning: State " << state << " not in Q-table." << endl;
				qTable[state].direction = vector<double>(4, 0.0);
			}

			// Choose the best action according to Q-values
			auto &qValues = qTable[state].direction;
			int bestAction = max_element(qValues.begin(), qValues.end()) - qValues.begin();
			char direction = DIRECTIONS[bestAction];

			cout << "Step " << steps << ": Moving " << direction << " from "
					<< currentPos.first << "," << currentPos.second << endl;

			// Take action
			auto [newPos, reward] = GridAPI::makeMove(direction);

			// Check if target found
			if (reward >= 1000) {
				foundTarget = true;
				cout << "Target reached after " << steps << " steps!" << endl;
				break;
			}

			// Update state and position
			state = stateToString(newPos);
			currentPos = newPos;

			cout << "asleep..";
			std::this_thread::sleep_until(wait_until);
			cout << "awake.. ";
		}

		if (!foundTarget) {
			cout << "Failed to find target within " << maxSteps << " steps." << endl;
		}
	}

	// Train the Q-learner
	void train(int episodes = 100) {
		cout << "Starting Q-learning training for " << episodes << " episodes..." << endl;

		for (int i = episodeCount; i < episodes; ++i) {
			runEpisode();
		}

		cout << "Training complete." << endl;
	}

	// Visualize the learned policy for a region of the grid
	void visualizePolicy(int radius = 5, int centerX = -1, int centerY = -1) {
		if (centerX < 0) centerX = currentPos.first;
		if (centerY < 0) centerY = currentPos.second;

		int minX = max(0, centerX - radius);
		int maxX = min(GRID_SIZE - 1, centerX + radius);
		int minY = max(0, centerY - radius);
		int maxY = min(GRID_SIZE - 1, centerY + radius);

		cout << "Policy visualization (region [" << minX << "," << minY
				<< "] to [" << maxX << "," << maxY << "]):" << endl;

		// Print column headers
		cout << "   ";
		for (int j = minY; j <= maxY; ++j) {
			cout << (j % 10) << "  ";
		}
		cout << endl;

		// Print grid with best actions
		for (int i = minX; i <= maxX; ++i) {
			// Print row header
			cout << (i % 10) << ": ";

			for (int j = minY; j <= maxY; ++j) {
				string state = to_string(i) + "," + to_string(j);

				if (i == currentPos.first && j == currentPos.second) {
					cout << "C  "; // Current position
				} else if (qTable.find(state) != qTable.end()) {
					auto &qValues = qTable[state].direction;
					int bestAction = max_element(qValues.begin(), qValues.end()) - qValues.begin();
					char actionChar;
					switch (bestAction) {
					case N:
						actionChar = '^';
						break;
					case E:
						actionChar = '>';
						break;
					case S:
						actionChar = 'v';
						break;
					case W:
						actionChar = '<';
						break;
					default:
						actionChar = '?';
					}
					cout << actionChar << "  ";
				} else {
					cout << ".  "; // Unknown
				}
			}
			cout << endl;
		}
	}

	// Adjust learning parameters
	void setParameters(double newAlpha, double newGamma, double newEpsilon,
					double newEpsilonDecay, double newMinEpsilon) {
		alpha = newAlpha;
		gamma = newGamma;
		// epsilon = newEpsilon;
		// minEpsilon = newMinEpsilon;
		epsilonDecay = newEpsilonDecay;

		cout << "Learning parameters updated:" << endl;
		cout << "Alpha (learning rate): " << alpha << endl;
		cout << "Gamma (discount factor): " << gamma << endl;
		// cout << "Epsilon (exploration rate): " << epsilon << endl;
		cout << "Epsilon decay rate: " << epsilonDecay << endl;
		cout << "Minimum epsilon: " << minEpsilon << endl;
	}
};

int main(int argc, char** argv) {
	int world1 = 1, userid1 = 3671, teamid1 = 1460, train1 = 200, modify1 = 0, target1 = 0;
	// int always1 = 0;
	// int alpha0, eps0, tau0;
	// bool feature = false, boltzman = false;

	std::string argument = (argc > 1) ? (argv[1]) : ("-help");

	std::cout << "total arguments: " << int((argc - 1) / 2) << "\n";
	if (argument == "-help") {
		std::cout << "\nneed at least one argument to start like: -world 1\n";
		std::cout << "-userid {default(3671)}\n";
		std::cout << "-teamid {default(1447)}\n";
		std::cout << "-time {seconds before making new move. default(6)}\n";
		std::cout << "-world {which world we learning. default(1)}\n";
		std::cout << "-train {how much episode to train. 0 to skip when target is found. default(200)}\n";
		std::cout << "-modify {1 - modifies qvalues towards target. 2 - cleans previus training. default(0)}\n";
		std::cout << "-target {1 - hopefully goes towards target. default(0)}\n";
		return 0;
	}

	argc--;
	for (int i = 1; i < argc; i += 2) {
		std::string argument = argv[i];
		std::cout << argument << " " << argv[i + 1] << "\n";
		if (false) ;
		else if (argument == "-userid") userid1 = std::stoi(argv[i + 1]);
		else if (argument == "-teamid") teamid1 = std::stoi(argv[i + 1]);
		else if (argument == "-world") world1 = std::stoi(argv[i + 1]);
		else if (argument == "-train") train1 = std::stoi(argv[i + 1]);
		else if (argument == "-modify") modify1 = std::stoi(argv[i + 1]);
		else if (argument == "-target") target1 = std::stoi(argv[i + 1]);
		else if (argument == "-time") TIME_DELAY = std::stoi(argv[i + 1]);
		else {
			std::cout << "Error with param:{" << argument << "}\n";
			return -1;
		}
	}

	GridAPI::teamid1 = teamid1;
	GridAPI::userid1 = userid1;
	GridAPI::worldid1 = world1;
	GridAPI::readyH();

	cout << "Starting Q-Learning Grid Explorer..." << endl;
	QLearningSolver solver;
	solver.setParameters(0.2, 0.95, 0.5, 0.995, 0.01);
	solver.loadQTable(); // or solver.modifyQTableForTarget();

	if (train1 > 0) solver.train(train1); // Train for 200 episodes
	if (modify1 > 0) solver.modifyQTableForTarget(modify1 == 2); // Train for target location
	if (target1) solver.findOptimalPath(); // Find optimal path to the target

	// Visualize the learned policy
	solver.visualizePolicy(8);

	cout << "Program complete." << endl;
	return 0;
}