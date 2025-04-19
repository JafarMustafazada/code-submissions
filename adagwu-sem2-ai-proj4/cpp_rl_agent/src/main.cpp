#include "jdevtools/curlcmd.hpp"
#include "nlohmann/json.hpp"

// #include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

using json = nlohmann::json;
using namespace curlcmd;
using namespace std;

static constexpr int TEAM_ID = 1447;
static constexpr int NUM_WORLDS = 10;
static constexpr int VISITS_PER_WORLD = 5;
static constexpr int STEPS_PER_VISIT = 1600;
static constexpr int A = 4; // N,E,S,W

// main algorithm
struct QLearner {
	int S; // number of states in current world
	double alpha = 0.1, gamma = 0.99;
	double eps = 1.0, eps_min = 0.1, eps_decay = 0.999;
	vector<vector<double> > Q; // Q[s][a]

	QLearner(int S_) : S(S_), Q(S_, vector<double>(A, 0.0)) {
		// optimistic init
		for (auto &row : Q)
			for (auto &q : row)
				q = 1.0;
	}

	int choose(int s) {
		if (double(rand()) / RAND_MAX < eps) return rand() % A;
		auto &row = Q[s];
		return max_element(row.begin(), row.end()) - row.begin();
	}

	void update(int s, int a, int sp, double r) {
		double bestp = *max_element(Q[sp].begin(), Q[sp].end());
		Q[s][a] += alpha * (r + gamma * bestp - Q[s][a]);
	}

	void decay_epsilon() {
		eps = max(eps_min, eps * eps_decay);
	}
};

// json problem solving squad
template <typename T>
T json_getter(const json &j, const std::string &key, T asDefault) {
	if (!j.contains(key) || j[key].is_null()) return asDefault;
	const auto &value = j.at(key);
	if (value.is_string()) return std::stol<T>(value.get<std::string>());
	if (value.is_number()) return value.get<T>();
	throw std::string("Type must be number or string, but is " + value.type_name() + " for key: " + key);
}

template <>
int json_getter<int>(const json &j, const std::string &key, int asDefault) {
	if (!j.contains(key) || j[key].is_null() || j[key].empty()) return asDefault;
	std::string val = j.at(key).get<std::string>();
	if (!val.size()) return asDefault;
	return std::stoi(val);
}

template <>
double json_getter<double>(const json &j, const std::string &key, double asDefault) {
	if (!j.contains(key) || j[key].is_null()) return asDefault;
	std::string val = j.at(key).get<std::string>();
	if (!val.size()) return asDefault;
	return std::stod(val);
}

int main() {
	srand(time(nullptr));

	std::string apikey = "";
	try {
		std::ifstream file("apikey.txt");
		std::getline(file, apikey);
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	vector<string> haeders = {
		"Content-Type: application/x-www-form-urlencoded",
		"x-api-key: " + apikey,
		"userId: 3671"
	};

	// to track how many times we've visited each world
	vector<int> visits(NUM_WORLDS, 0);

	for (int world = 0; world <= NUM_WORLDS; world++) {
		while (visits[world] < VISITS_PER_WORLD) {
			//
			// 1) If not in any world
			//
			try {
				{
					// get current location
					requestData req;
					req.headers = haeders;
					req.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php?type=location&teamId=" + to_string(TEAM_ID);
					json loc_js = json::parse(sender(req));
					cout << loc_js << "\n";
					int curWorld = json_getter(loc_js, "world", -1);
					if (curWorld != world) {
						// enter
						requestData e;
						e.headers = haeders;
						e.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php";
						e.postData = "type=enter&worldId=" + to_string(world) + "&teamId=" + to_string(TEAM_ID);
						cout << sender(e, true) << '\n';
					}
				}
			} catch (const std::exception &e) {
				std::cout << "error1.\n";
				std::cerr << e.what() << '\n';
			}

			//
			// 2) Q‑Learning in this world
			//
			int S = 40 * 40;
			QLearner ql(S);
			int state;

			try {
				{
					requestData req;
					req.headers = haeders;
					req.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php?type=location&teamId=" + to_string(TEAM_ID);
					json js = json::parse(sender(req));
					cout << js << "\n";
					state = json_getter(js, "state", 0);
				}
			} catch (const std::exception &e) {
				std::cout << "error2.\n";
				std::cerr << e.what() << '\n';
			}

			// making moves every 15 second
			try {
				for (int step = 0; step < STEPS_PER_VISIT; step++) {
					auto wait_until = std::chrono::system_clock::now() + std::chrono::seconds(15);

					// pick action
					int a = ql.choose(state);
					static const vector<string> acts = {"N", "E", "S", "W"};

					// perform move
					requestData mv;
					mv.headers = haeders;
					mv.url = "https://www.notexponential.com/aip2pgaming/api/rl/gw.php";
					mv.postData = "type=move" + string("&teamId=") + to_string(TEAM_ID) + string("&worldId=") + to_string(world) + string("&move=") + acts[a];

					json mv_js = json::parse(sender(mv, true));
					cout << mv.postData << "\n";
					cout << mv_js << "\n";

					double reward = json_getter(mv_js, "reward", 0.0);
					int nextState = json_getter(mv_js, "newState", 0);

					// update Q
					ql.update(state, a, nextState, reward);
					ql.decay_epsilon();

					state = nextState;
					std::this_thread::sleep_until(wait_until);
				}
				visits[world]++;
			} catch (const std::exception &e) {
				std::cout << "error3.\n";
				std::cerr << e.what() << '\n';
			}

			cout << "Completed visit " << visits[world] << " of world " << world << "\n";
		}
	}

	// 3) retrieve overall score
	{
		requestData req;
		req.headers = haeders;
		req.url = "https://www.notexponential.com/aip2pgaming/api/rl/score.php?type=score&teamId=" + to_string(TEAM_ID);
		json js = json::parse(sender(req));
		cout << "Total score: " << js.value("score", 0) << "\n";
	}

	return 0;
}
