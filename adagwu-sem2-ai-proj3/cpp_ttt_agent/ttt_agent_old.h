#include <vector>
#include <unordered_map>
#include <string>
#include <thread>

using namespace std;

namespace ttt_agent {
	struct pii { int i; int j; };
	struct piii { pii best_move = {-1, -1}; int best_score = INT_MIN; };
	const char X = 'X', O = 'O', E = '.';
	const int WIN_SCORE = 10;

	struct Board {
		vector<vector<char> > g;
		int n, m;
		pii lastmove = {-1, -1};

		Board(int n, int m): n(n), m(m) { g.assign(n, vector<char>(n, E)); }
		string display() const {
			static vector<string> numbers;
			string result = "rc ";
			if (!numbers.size()) {
				for (int i = 0; i < n; i++) {
					if (i < 10) numbers.push_back((to_string(i) + "  "));
					else numbers.push_back((to_string(i) + " "));
				}
				numbers.push_back("\n");
			}

			for (int i = 0; i <= n; i++) result.append(numbers[i]);
			for (int i = 0; i < n; i++) {
				result.append(numbers[i]);
				for (auto c: g[i]) result.push_back(c), result.push_back(' '), result.push_back(' ');
				result.push_back('\n');
			}
			
			result.push_back('\n');
			return result;
		}
		bool full() const {
			for(auto &r: g) for(auto c: r) if(c==E)return false; return true;
		}
		string boardKey() const {
			string s;
			for (auto &r: g) for(auto c: r) s.push_back(c);
			return s;
		}
		vector<pii> moves() const {
			vector<pii> mv;
			for(int i=0;i<n;i++) for(int j=0;j<n;j++) if(g[i][j]==E) mv.push_back({i,j});
			return mv;
		}
		void make(pii move, char p) { g[move.i][move.j] = p; lastmove = move; }
		void undo(pii move) { g[move.i][move.j] = E; }
		bool isMoveWin(const pii &move) const {
			int dirs[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
			char p = g[move.i][move.j];
			for (auto &d: dirs) {
				int cnt = 1;
				pii line = move;
				for (int i = 1; i < m; i++) {
					line.i += d[0];
					line.j += d[1];
					if (line.i < 0 || line.j < 0 || line.i >= n || line.j >= n) break;
					if (g[line.i][line.j] == p) cnt++;
					else break;
				}
	
				d[0] = -d[0];
				d[1] = -d[1];
				line = move;
				for (int i = 1; i < m; i++) {
					line.i += d[0];
					line.j += d[1];
					if (line.i < 0 || line.j < 0 || line.i >= n || line.j >= n) break;
					if (g[line.i][line.j] == p) cnt++;
					else break;
				}
	
				if (cnt >= m) return true;
			}
			return false;
		}

		bool inBounds(int i, int j) const { return (i >= 0 && j >= 0 && i < n && j < n); }


		vector<vector<char> > g;
		// returns `0` if there no danger for oponent of this move
		// gives number of `1 step from winning` scenarios for this move (maximum `8` as per number of directions)
		// `-1` means move is game winning and therefor game ending one.
		int checkAllDangers(const pii &move, pii &endpoint2) const {
			int dirs[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
			int dangers = 0;
			for (auto &dir: dirs) {
				int danger = moveDirectionDangers(move, endpoint2, dir[0], dir[1]);
				if (danger == -1) return danger;
				dangers += danger;
			}
			return dangers;
		}
		int moveDirectionDangers(const pii &move, pii &endpoint2, int dx, int dy) const {
			int cnt = 1, dangers = 0, total, px, py, i;
			pii endpoint1 = {-1, -1};
			char p = g[move.i][move.j];
			int isgap = 0;

			px = move.i; py = move.j;
			for (i = 1; i < m; i++) {
				px += dx; py += dy;
				if (!inBounds(px, py)) break;
				if (g[px][py] == p) cnt++;
				else {
					if (g[px][py] == E) endpoint1 = {px, py}, isgap |= 1;
					break;
				}
			}

			total = cnt;
			if (isgap & 1) {
				for (i++; i < m; i++) {
					px += dx; py += dy;
					if (!inBounds(px, py)) break;
					if (g[px][py] == p) total++;
					else break;
				}
			}

			if (total + 1 >= m && isgap) {
				dangers++;
				endpoint2 = endpoint1;
			}

			dx = -dx; dy = -dy;
			px = move.i; py = move.j;
			for (i = 1; i < m; i++) {
				px += dx; py += dy;
				if (!inBounds(px, py)) break;
				if (g[px][py] == p) cnt++;
				else {
					if (g[px][py] == E) endpoint1 = {px, py}, isgap |= 2;
					break;
				}
			}
			
			total = cnt;
			if (isgap & 2) {
				for (i++; i < m; i++) {
					px += dx; py += dy;
					if (!inBounds(px, py)) break;
					if (g[px][py] == p) total++;
					else break;
				}
			}

			if (total + 1 >= m && isgap) {
				dangers++;
				endpoint2 = endpoint1;
			}

			if (cnt >= m) return -1;
			return dangers;
		}
	};


	// unused yet
	// static int evalBoard0(const Board &B, const pii &move, char player) { return 0; }
	// static int evalBoard1(const Board &B, const pii &move, char player) {
	// 	int score = 0;
	// 	int center = B.n/2;
	// 	char opp = (player == X ? O : X);
	// 	for (int i = 0; i < B.n; i++){
	// 		for (int j = 0; j < B.n; j++){
	// 			if (B.g[i][j]==player){
	// 				// first player favors edges
	// 				if (i==center && j==center) score += 100; 
	// 				else if(i==0 || j==0 || i==B.n-1 || j==B.n-1) score += (player==X ? 50 : 0);
	// 			}
	// 			if (B.g[i][j]==opp){
	// 				if (i==center && j==center) score -= 100;
	// 			}
	// 		}
	// 	}
	// 	return score;
	// }


	class aiPlayer {
		char human = X;
		char player = O;
		pii mylastmove = {-1, -1};
		unordered_map<string, int> cache;
		// int (*evalMethod)(const Board &B, const pii &move, char player) = evalBoard0;

		int minimax(Board &B, const pii &move, int depth = 4, bool ismin = true) {
			string key = B.boardKey();
			if (cache.count(key)) return cache[key];

			int danger = B.checkAllDangers(move, pii());
			if (danger == -1) return (ismin ? WIN_SCORE : -WIN_SCORE);
			if (depth == 0 || B.full()) return (ismin ? danger : -danger);

			int best = ismin ? INT_MAX : INT_MIN;
			for (int i = 0; i < B.n; i++) {
				for (int j = 0; j < B.n; j++) {
					if (B.g[i][j] != E) continue;
					pii nmove = {i, j};
					B.make(nmove, (ismin ? human : player));
					int score = minimax(B, nmove, depth - 1, !ismin);
					B.undo(nmove);
					if (ismin) best = (best > score) ? score : best;
					else best = (best < score) ? score : best;
				}
			}

			cache[key] = best;
			return best;
		}

		int minimax_alpha_beta(Board &B, const pii &move, 
			int depth = 4, bool ismin = true, int alpha = INT_MIN, int beta = INT_MAX) {
			string key = B.boardKey();
			if (cache.count(key)) return cache[key];

			int danger = B.checkAllDangers(move, pii());
			if (danger == -1) return (ismin ? WIN_SCORE : -WIN_SCORE);
			if (depth == 0 || B.full()) return (ismin ? danger : -danger);

			int best = ismin ? INT_MAX : INT_MIN;
			for (int i = 0; i < B.n; i++) {
				for (int j = 0; j < B.n; j++) {
					if (B.g[i][j] != E) continue;

					pii nmove = {i, j};
					B.make(nmove, (ismin ? human : player));
					int score = minimax_alpha_beta(B, nmove, depth - 1, !ismin, alpha, beta);
					B.undo(nmove);

					if (ismin) {
						if (score < best) best = score;
						if (best < beta) beta = best;
					}
					else {
						if (score > best) best = score;
						if (best > alpha) alpha = best;
					}

					if (beta <= alpha) {
						cache[key] = best;
						return best;
					}
				}
			}

			cache[key] = best;
			return best;
		}

	public:
	aiPlayer(char player = O) {
			this->player = player;
			human = (player == O ? X : O);
			// this->evalMethod = evalMethod;
		}

		char getC() { return player; }

		pii minimaxBest(Board &B, bool alphabeta = false, int depth = 4, bool clearCache = true) {
			int best_score = INT_MIN;
			pii best_move = {-1, -1}, danger_pos {-1, -1};

			if (B.lastmove.i == -1) {
				mylastmove = {0, 0};
				return mylastmove;
			}

			int danger = B.lastmove.i == -1 ? 0 : B.checkAllDangers(B.lastmove, danger_pos);
			if (danger) best_move = danger_pos;
			
			int danger2 = mylastmove.i == -1 ? 0 : B.checkAllDangers(mylastmove, danger_pos);
			if (danger2) best_move = danger_pos;
			
			if (danger || danger2) {
				mylastmove = best_move;
				return best_move;
			}

			for (int i = 0; i < B.n; i++) {
				for (int j = 0; j < B.n; j++) {
					if (B.g[i][j] != E) continue;
					pii nmove = {i, j};
					B.make(nmove, player);
					int score = (alphabeta ? 
						minimax_alpha_beta(B, nmove, depth) : 
						minimax(B, nmove, depth));
					B.undo(nmove);
					if (score > best_score) {
						best_score = score;
						best_move = nmove;
					}
				}
			}
			
			if (clearCache) cache.clear();
			mylastmove = best_move;
			return best_move;
		}

		vector<vector<pii> > get_jobes(Board &B, int workers = 1) {
			vector<vector<pii> > jobs(workers);
			int counter = 0;
			for (int i = 0; i < B.n; i++) {
				for (int j = 0; j < B.n; j++) {
					if (B.g[i][j] != E) continue;
					jobs[counter++].push_back({i, j});
					if (counter == workers) counter = 0;
				}
			}
			return jobs;
		}

		pii minimaxBest_threading(Board &B, bool alphabeta = false, int depth = 4, bool clearCache = true, int numThreads = 1) {
			int best_score = INT_MIN;
			pii best_move = {-1, -1}, danger_pos {-1, -1};

			if (B.lastmove.i == -1) {
				mylastmove = {0, 0};
				return mylastmove;
			}

			int danger = B.lastmove.i == -1 ? 0 : B.checkAllDangers(B.lastmove, danger_pos);
			if (danger) best_move = danger_pos;
			
			int danger2 = mylastmove.i == -1 ? 0 : B.checkAllDangers(mylastmove, danger_pos);
			if (danger2) best_move = danger_pos;
			
			if (danger || danger2) {
				mylastmove = best_move;
				return best_move;
			}

			vector<thread> threads;
			threads.reserve(numThreads);

			vector<vector<pii> > jobs = get_jobes(B, numThreads);
			vector<piii> tools(numThreads);
			vector<aiPlayer> players(numThreads, *this);
			vector<Board> boards(numThreads, B);

			for (int i = 0; i < numThreads; i++) {
				vector<pii>& job = jobs[i];
				piii& tool = tools[i];
				aiPlayer& p1 = players[i];
				Board& B2 = boards[i];
				threads.emplace_back([&job, &B2, &p1, &tool, depth, alphabeta] {
					for (auto& move: job) {
						B2.make(move, p1.player);
						int score = (alphabeta ? 
							p1.minimax_alpha_beta(B2, move, depth) : 
							p1.minimax(B2, move, depth));
						B2.undo(move);
						if (score > tool.best_score) {
							tool.best_score = score;
							tool.best_move = move;
						}
					}
				});
			}
			for (auto &t: threads) {
				if (t.joinable()) t.join();
			}
			for (int i = 0; i < jobs.size(); i++) {
				if (tools[i].best_score > best_score) {
					best_score = tools[i].best_score;
					best_move = tools[i].best_move;
				}
			}

			if (clearCache) cache.clear();
			mylastmove = best_move;
			return best_move;
		}
	
		void filterCache(Board &B) {
			string key = B.boardKey();
			for (auto node: cache) {
				for (int i = 0; i < key.size(); i++) {
					if (key[i] == E) continue;
					if (key[i] == node.first[i]) continue;
					cache.erase(key);
					break;
				}
			}
		}
	};
}