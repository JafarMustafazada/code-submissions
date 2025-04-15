
#include <stdexcept>
#include <cstdio>
#include <string>
#include <unordered_map>

namespace curlcmd {
	#if defined(_WIN32)
	#define popen _popen
	#define pclose _pclose
	#endif
	
	std::string exec(const char* cmd) {
		char buffer[128];
		std::string result = "";
		FILE* pipe = popen(cmd, "r");
		if (!pipe) throw std::runtime_error("popen() failed!");
		try {
			while (fgets(buffer, sizeof buffer, pipe) != NULL) {
				result += buffer;
			}
		} catch (...) {
			pclose(pipe);
			throw;
		}
		pclose(pipe);
		return result;
	}

	struct requestData {
		std::string url = "example.com";
		std::vector<std::string> headers;
		// url encoded data
		std::string postData = "";
		// data to be url encoded
		std::vector<std::string> urlEncodeData;
	};

	std::string sender(requestData &req) {
		std::string command = "cur --location \"" + req.url + '"';
		for (int i = 0; i < req.headers.size(); i++) {
			command += " --header \"" + req.headers[i] + '"';
		}
		if (req.postData.size()) command += " -d \"" + req.postData + '"';
		for (int i = 0; i < req.urlEncodeData.size(); i++) {
			command += " --data-urlencode \"" + req.urlEncodeData[i] + '"';
		}
		return exec(command.data());
	} 
}