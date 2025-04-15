#include <vector>
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

namespace ttt_agent {
	struct pii { int i; int j; };
	struct piii { pii best_move = {-1, -1}; int best_score = INT_MIN; };
	const char X = 'X', O = 'O', E = '.';
	const int WIN_SCORE = 100;
	const int LOSE_SCORE = -100;

	struct Board {
		std::vector<std::vector<char> > g;
		pii lastmove = {-1, -1};
		int n, m, size, center;

		Board(int n, int m): n(n), m(m) { g.assign(n, std::vector<char>(n, E)); size = n * n; center = (n >> 1); }

		std::string display() const {
			static std::vector<std::string> numbers;
			std::string result = "rc ";
			if (!numbers.size()) {
				for (int i = 0; i < n; i++) {
					if (i < 10) numbers.push_back((std::to_string(i) + "  "));
					else numbers.push_back((std::to_string(i) + " "));
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
		std::string boardKey() const {
			std::string s;
			s.reserve(size);
			for (auto &r: g) for(auto c: r) s.push_back(c);
			return s;
		}

		void make(pii move, char p) { g[move.i][move.j] = p; lastmove = move; }
		void undo(pii move) { g[move.i][move.j] = E; }

		bool inBounds(int i, int j) const { return (i >= 0 && j >= 0 && i < n && j < n); }
		bool full() const {
			for(auto &r: g) for(auto c: r) if(c==E)return false; return true;
		}
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

		// returns `0` if there no danger for oponent of this move
		// gives number of `1 step from winning` scenarios for this move (maximum `8`, as per number of directions, yet more than 2 is already game ending move)
		// gives `INT_MAX` instead when move is game winning and therefor game ending one.
		int checkAllDangers(const pii &move, pii &endpoint, char p = E) const {
			int dirs[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
			int dangers = 0;

			if (p == E) p = g[move.i][move.j];
			for (auto &dir: dirs) {
				int danger = moveDirectionDangers(move, endpoint, dir[0], dir[1], p);
				if (danger == -1) return INT_MAX;
				dangers += danger;
			}

			if (dangers >= 2) return INT_MAX;
			return dangers;
		}
		int moveDirectionDangers(const pii &move, pii &endpoint2, int dx, int dy, char p = E) const {
			int cnt = 1, dangers = 0, total, px, py, i;
			pii endpoint1 = {-1, -1};
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

		// Lower Manhattan distance from the center gets a higher (less negative) is value.
		int heuristic2(const pii &move) const {
			int di = abs(move.i - center);
			int dj = abs(move.j - center);
			return -(di + dj);
		}

		std::vector<pii> getCandidateMoves(int radius = 1) const {
			std::vector<pii> candidates;
			std::vector<std::vector<bool> > g2(n, std::vector<bool>(n, false));
	
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					if (g[i][j] == E) continue;					

					for (int  di = -radius; di <= radius; di++) {
						for (int dj = -radius; dj <= radius; dj++) {
							int ni = i + di, nj = j + dj;
							if (!inBounds(ni, nj) || g2[ni][nj] || g[ni][nj] != E) continue;
							g2[ni][nj] = 1;
							candidates.push_back({ni, nj});
						}
					}
				}
			}

			sort(candidates.begin(), candidates.end(), [this](const pii &a, const pii &b) {
				return heuristic2(a) > heuristic2(b);
				// static char p = g[lastmove.i][lastmove.j] == X ? O : X;
				// return checkAllDangers(a, pii(), p) > checkAllDangers(b, pii(), p);
			});
			return candidates;
		}
	};

	struct playerData {
		Board B;
		int depth = 10;
		char opp = X;
		char car = O;
		pii mylastmove = {-1, -1};
		std::unordered_map<std::string, int> cache;
	};


	int minimax_alpha_beta(playerData& data, const pii &move, int depth, 
		std::chrono::time_point<std::chrono::high_resolution_clock> start, int limit = 30000, 
		bool ismin = true, int alpha = INT_MIN, int beta = INT_MAX) {
		std::string key = data.B.boardKey();
		if (data.cache.count(key)) return data.cache[key];

		int danger = data.B.checkAllDangers(move, pii());
		if (danger == INT_MAX) return (ismin ? WIN_SCORE : LOSE_SCORE);
		if (depth == 0 || data.B.full()) return (ismin ? danger : -danger);

		int best = ismin ? INT_MAX : INT_MIN;
		std::vector<pii> moves = data.B.getCandidateMoves();
		for (pii &nmove: moves) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
			if (elapsed >= limit) break;
			
			data.B.make(nmove, (ismin ? data.opp : data.car));
			int score = minimax_alpha_beta(data, nmove, depth - 1, start, limit, !ismin, alpha, beta);
			data.B.undo(nmove);

			if (ismin) {
				if (score < best) best = score;
				if (best < beta) beta = best;
			}
			else {
				if (score > best) best = score;
				if (best > alpha) alpha = best;
			}
			if (beta <= alpha) break;
		}

		data.cache[key] = best;
		return best;
	}


	int minimax(playerData& data, const pii &move, int depth, 
		std::chrono::time_point<std::chrono::high_resolution_clock> start, int limit = 30000, bool ismin = true) {
		std::string key = data.B.boardKey();
		if (data.cache.count(key)) return data.cache[key];

		int danger = data.B.checkAllDangers(move, pii());
		if (danger == INT_MAX) return (ismin ? WIN_SCORE : LOSE_SCORE);
		if (depth == 0 || data.B.full()) return (ismin ? danger : -danger);

		int best = ismin ? INT_MAX : INT_MIN;
		std::vector<pii> moves = data.B.getCandidateMoves();
		for (pii &nmove: moves) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
			if (elapsed >= limit) break;

			data.B.make(nmove, (ismin ? data.opp : data.car));
			int score = minimax(data, nmove, depth - 1, start, limit, !ismin);
			data.B.undo(nmove);

			if (ismin) {
				if (score < best) best = score;
			}
			else {
				if (score > best) best = score;
			}
		}

		data.cache[key] = best;
		return best;
	}


	pii minimaxBest(playerData &p, bool alphabeta = true, int timeLimitMs = 30000) {
		auto start = std::chrono::high_resolution_clock::now();
		pii nmove;
		piii global1;

		if (p.B.lastmove.i == -1) {
			p.mylastmove = {p.B.n / 2, p.B.n / 2};
			return p.mylastmove;
		}

		int danger = p.B.lastmove.i == -1 ? 0 : p.B.checkAllDangers(p.B.lastmove, nmove);
		if (danger) global1.best_move = nmove;
		
		int danger2 = p.mylastmove.i == -1 ? 0 : p.B.checkAllDangers(p.mylastmove, nmove);
		if (danger2) global1.best_move = nmove;
		
		if (danger || danger2) {
			p.mylastmove = global1.best_move;
			return global1.best_move;
		}
		
		std::vector<pii> moves = p.B.getCandidateMoves();
		long long elapsed;
		for (pii &move: moves) {
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
			if (elapsed >= timeLimitMs) {
				p.depth--;
				break;
			}
			p.B.make(move, p.car);
			int score = (alphabeta ? 
				minimax_alpha_beta(p, move, p.depth, start, timeLimitMs) : 
				minimax(p, move, p.depth, start, timeLimitMs));
			p.B.undo(move);
			if (score > global1.best_score) {
				global1.best_score = score;
				global1.best_move = move;
			}
		}
		if (elapsed < (timeLimitMs >> 3)) p.depth++;
		
		p.cache.clear();
		p.mylastmove = global1.best_move;
		return global1.best_move;
	}
	
	struct thread_job {
		playerData p;
		piii local1;
		std::vector<pii> moves;
	};

	std::vector<thread_job> get_jobes(playerData &p, int workers = 1) {
		std::vector<thread_job> jobs(workers, thread_job{p});
		int counter = 0;

		for (pii &move: p.B.getCandidateMoves()) {
			jobs[counter++].moves.push_back(move);
			if (counter == workers) counter = 0;
		}

		return jobs;
	}

	pii minimaxBestThreading(playerData &p, bool alphabeta = true, int threadNum = 1, int timeLimitMs = 30000) {
		pii nmove;
		piii global1;

		if (p.B.lastmove.i == -1) {
			p.mylastmove = {p.B.n / 2, p.B.n / 2};
			return p.mylastmove;
		}

		int danger = p.B.lastmove.i == -1 ? 0 : p.B.checkAllDangers(p.B.lastmove, nmove);
		if (danger) global1.best_move = nmove;
		
		int danger2 = p.mylastmove.i == -1 ? 0 : p.B.checkAllDangers(p.mylastmove, nmove);
		if (danger2) global1.best_move = nmove;
		
		if (danger || danger2) {
			p.mylastmove = global1.best_move;
			return global1.best_move;
		}

		std::vector<std::thread> threads;
		threads.reserve(threadNum);
		std::vector<thread_job> jobs = get_jobes(p, threadNum);
		auto start = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < threadNum; i++) {
			thread_job &job = jobs[i];
			
			threads.emplace_back([&job, timeLimitMs, start, alphabeta] {
				long long elapsed;
				for (auto& move: job.moves) {
					elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
					if (elapsed >= timeLimitMs) {
						job.p.depth--;
						break;
					}
					job.p.B.make(move, job.p.car);
					int score = (alphabeta ? 
						minimax_alpha_beta(job.p, move, job.p.depth, start, timeLimitMs) : 
						minimax(job.p, move, job.p.depth, start, timeLimitMs));
					job.p.B.undo(move);
					if (score > job.local1.best_score) {
						job.local1.best_score = score;
						job.local1.best_move = move;
					}
				}
				if (elapsed < (timeLimitMs >> 3)) job.p.depth++;
			});
		}
		for (auto &t: threads) {
			if (t.joinable()) t.join();
		}
		bool islate = false;
		bool isfast = true;
		for (int i = 0; i < jobs.size(); i++) {
			if (jobs[i].local1.best_score > global1.best_score) {
				global1.best_score = jobs[i].local1.best_score;
				global1.best_move = jobs[i].local1.best_move;
			}
			if (!islate && jobs[i].p.depth < p.depth) islate = true;
			if (isfast && jobs[i].p.depth == p.depth) isfast = false;
		}
		if (islate) p.depth--;
		else if (isfast) p.depth++;


		p.cache.clear();
		p.mylastmove = global1.best_move;
		return global1.best_move;
	}
}