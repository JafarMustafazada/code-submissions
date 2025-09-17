#include "oglproj1.h" // adds helping Shader and Model class; includes glm, iostream, fstream, sstream

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>

// Motion controller!
enum OrientationType {
	Quaternion,
	Euler
};
enum InterpType {
	CatmullRom,
	BSpline
};

struct Keyframe {
	glm::vec3 position;
	glm::vec3 euler; // Euler angles in radians
	glm::quat quat;  // Quaternion
};

class MotionController {
  public:
	std::vector<Keyframe> keys;
	OrientationType orientType = OrientationType::Quaternion;
	InterpType interpType = InterpType::CatmullRom;

	// Add a keyframe (Euler or Quaternion)
	void addKey(const glm::vec3 &pos, const glm::vec3 &euler) {
		Keyframe k;
		k.position = pos;
		k.euler = euler;
		k.quat = glm::quat(euler);
		keys.push_back(k);
	}
	void addKey(const glm::vec3 &pos, const glm::quat &quat) {
		Keyframe k;
		k.position = pos;
		k.quat = quat;
		k.euler = glm::eulerAngles(quat);
		keys.push_back(k);
	}

	glm::vec3 getPosition(int i, int &maxVal) const {
		int clampedIndex = glm::clamp(i, 0, maxVal);
		return keys[clampedIndex].position;
	};

	glm::vec3 interpolateCatmullRom(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, float t) const {
		float t2 = t * t;
		float t3 = t2 * t;

		glm::vec3 a = 2.0f * p1;
		glm::vec3 b = -p0 + p2;
		glm::vec3 c = 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3;
		glm::vec3 d = -p0 + 3.0f * p1 - 3.0f * p2 + p3;

		return 0.5f * (a + b * t + c * t2 + d * t3);
	}

	glm::vec3 interpolateBSpline(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, float t) const {
		float t2 = t * t;
		float t3 = t2 * t;
		float oneMinusT = 1.0f - t;

		float basis0 = oneMinusT * oneMinusT * oneMinusT / 6.0f;
		float basis1 = (3.0f * t3 - 6.0f * t2 + 4.0f) / 6.0f;
		float basis2 = (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) / 6.0f;
		float basis3 = t3 / 6.0f;

		return basis0 * p0 + basis1 * p1 + basis2 * p2 + basis3 * p3;
	}

	glm::vec3 interpPos(float t) const {
		int numKeys = keys.size();
		if (numKeys == 0) return glm::vec3(0);
		if (numKeys == 1) return keys[0].position;

		// Map t from [0,1] to segment range
		int maxVal = numKeys - 1;
		float segmentFloat = t * maxVal;
		int segmentIndex = glm::clamp(int(segmentFloat), 0, numKeys - 2);
		float localT = segmentFloat - segmentIndex;

		// Get control points for spline
		glm::vec3 p0 = getPosition(segmentIndex - 1, maxVal);
		glm::vec3 p1 = getPosition(segmentIndex, maxVal);
		glm::vec3 p2 = getPosition(segmentIndex + 1, maxVal);
		glm::vec3 p3 = getPosition(segmentIndex + 2, maxVal);

		if (interpType == InterpType::CatmullRom) return interpolateCatmullRom(p0, p1, p2, p3, localT);
		else return interpolateBSpline(p0, p1, p2, p3, localT);
	}

	// Interpolate orientation
	glm::quat interpQuat(float t) const {
		int numKeys = keys.size();
		if (numKeys == 0) return glm::quat();
		if (numKeys == 1) return keys[0].quat;

		float segmentFloat = t * (numKeys - 1);
		int segmentIndex = glm::clamp(int(segmentFloat), 0, numKeys - 2);
		float localT = segmentFloat - segmentIndex;

		if (orientType == OrientationType::Quaternion) {
			const glm::quat &startQuat = keys[segmentIndex].quat;
			const glm::quat &endQuat = keys[segmentIndex + 1].quat;
			return glm::slerp(startQuat, endQuat, localT);
		} else {
			const glm::vec3 &startEuler = keys[segmentIndex].euler;
			const glm::vec3 &endEuler = keys[segmentIndex + 1].euler;
			glm::vec3 interpolatedEuler = glm::mix(startEuler, endEuler, localT);
			return glm::quat(interpolatedEuler);
		}
	}

	// Get transform matrix at t (0..1)
	glm::mat4 getTransform(float t) const {
		glm::vec3 pos = interpPos(t);
		glm::quat rot = interpQuat(t);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		glm::mat4 R = glm::mat4_cast(rot);
		return T * R;
	}
};

// Globals
int W = 600, H = 600;
int angle = 0;
const int FPS = 60;
std::chrono::steady_clock::time_point lastTime;
std::unique_ptr<Shader> shader;
std::unique_ptr<Model> model;
glm::mat4 modelMat = glm::mat4(1.0f);
MotionController motion;
float motionTime = 0;

// middleware :P
void init(int argc, char **argv) {
	shader = std::make_unique<Shader>(vsSrc, fsSrc);
	model = std::make_unique<Model>();
	std::string fn = "teapot.obj";
	glClearDepth(1.0f);

	// Processing command line arguments
	for (int i = 1; i < argc; i++) {
		std::string args = argv[i];
		if (args == "-ot" && i + 1 < argc) {
			std::string t = argv[++i];
			if (t == "quat" || t == "quaternion" || t == "0") motion.orientType = OrientationType::Quaternion;
			else if (t == "euler" || t == "1") motion.orientType = OrientationType::Euler;
			else std::cerr << "Unknown orientation type: " << t << "\n";
			continue;
		} else if (args == "-it" && i + 1 < argc) {
			std::string t = argv[++i];
			if (t == "crspline" || t == "catmullrom" || t == "0") motion.interpType = InterpType::CatmullRom;
			else if (t == "bspline" || t == "b-spline" || t == "1") motion.interpType = InterpType::BSpline;
			else std::cerr << "Unknown interpolation type: " << t << "\n";
			continue;
		} else if (args == "-m" && i + 1 < argc) {
			fn = argv[++i];
			continue;
		} else if (args == "-h" || args == "--help") {
			std::cout << "Usage: oglproj1 [options]\n";
			std::cout << "Options:\n";
			std::cout << "  -ot <type> Orientation type: quat/quaternion/0 (default), euler/1\n";
			std::cout << "  -it <type> Interpolation type: crspline/catmullrom/0 (default), bspline/b-spline/1\n";
			std::cout << "  -m <file>  Load model from .obj file (default: teapot.obj)\n";
			std::cout << "  -h, --help Show this help message\n";
			exit(0);
		} else {
			std::cerr << "Unknown argument: " << args << "\n";
			std::cerr << "Use -h or --help for usage.\n";
			exit(1);
		}
	}

	if (!model->load(fn)) {
		std::cerr << "Failed to load model: " << fn << "\n";
		exit(1);
	}
	lastTime = std::chrono::steady_clock::now();

	motion.addKey(glm::vec3(0, 0, 0), glm::quat(glm::vec3(0, 0, 0)));
	motion.addKey(glm::vec3(2, 0, 0), glm::quat(glm::vec3(0, glm::radians(90.0f), 0)));
	motion.addKey(glm::vec3(2, 2, 0), glm::quat(glm::vec3(glm::radians(90.0f), glm::radians(90.0f), 0)));
	motion.addKey(glm::vec3(0, 2, 0), glm::quat(glm::vec3(glm::radians(180.0f), 0, 0)));
}

// animation happens here
void update() {
	auto now = std::chrono::steady_clock::now();
	static const int frameTime = 1000 / FPS;
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() >= frameTime) {
		// from exercise 0 //
		// angle = (angle + 5) % 360;
		// modelMat = glm::rotate(glm::mat4(1), glm::radians(float(angle)), glm::vec3(0, 1, 0));
		motionTime += 0.01f; // speed
		if (motionTime > 1.0f) motionTime = 0.0f;
		modelMat = motion.getTransform(motionTime);
		lastTime = now;
	}
}

void render() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	shader->use();
	shader->set("lightPos", glm::vec3(5, 5, 5));
	shader->set("lightAmbient", glm::vec3(0.4f));
	shader->set("lightDiffuse", glm::vec3(0.3f));
	shader->set("lightSpecular", glm::vec3(0.4f));
	shader->set("materialAmbient", glm::vec3(0.11f, 0.06f, 0.11f));
	shader->set("materialDiffuse", glm::vec3(0.43f, 0.47f, 0.54f));
	shader->set("materialSpecular", glm::vec3(0.33f, 0.33f, 0.52f));
	shader->set("materialEmission", glm::vec3(0.1f, 0, 0.1f));
	shader->set("materialShininess", 10.0f);
	shader->set("viewPos", glm::vec3(0, 0, 5));
	glm::mat4 view = glm::translate(glm::mat4(1), glm::vec3(0, 0, -5));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), float(W) / H, 1.0f, 2000.0f);
	shader->set("model", modelMat);
	shader->set("view", view);
	shader->set("projection", proj);
	shader->set("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMat))));
	model->draw();
}

// Input/resize
void key(GLFWwindow *w, int k, int, int, int) {
	if (k == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);
}
void resize(GLFWwindow *, int w, int h) {
	W = w;
	H = h;
	glViewport(0, 0, w, h);
}

int main(int argc, char **argv) {
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow *win = glfwCreateWindow(W, H, "OpenGLProject01", NULL, NULL);
	if (!win) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(win);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
	init(argc, argv);
	glfwSetKeyCallback(win, key);
	glfwSetFramebufferSizeCallback(win, resize);
	while (!glfwWindowShouldClose(win)) {
		update();
		render();
		glfwSwapBuffers(win);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}