#include "ComputeMain.h"

using namespace std;

extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

GLuint LoadVFShaders(const char * vertex_file_path, const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if (VertexShaderStream.is_open()) {
		string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if (FragmentShaderStream.is_open()) {
		string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

ComputeMain::ComputeMain(int argc, char **argv)
{
	// Initialize window with freeglut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(800, 600);
	glutCreateWindow("simFluid");

	// Output the currently used video card
	cout << glGetString(GL_VENDOR) << endl;
	cout << glGetString(GL_RENDERER) << endl;
	
	// Initialize GLEW and check the supported OpenGL version to run Compute Shader
	glewInit();
	if (glewIsSupported("GL_VERSION_4_3"))
		cout << "GL version is supported.\n";
	else
		cout << "GL version is not supported. Compute shader requires the minimum of OpenGL version 4.3. \n";
	currInstance = this;
	initRendering();
	glEnable(GL_DEPTH_TEST);
	glutDisplayFunc(draw);

	// Main loop to keep the window running
	glutMainLoop();
}


ComputeMain::~ComputeMain()
{
}

void ComputeMain::update(void)
{
	// Clears the buffer and paint the background black
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glClearColor(0.1f, 0.0f, 0.0f, 1.0);

	glActiveTexture(GL_TEXTURE0);

	particles->update();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// reference the compute shader buffer, which we will use for the particle
	// locations (only the positions for now)
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particles->getPosBuffer()->getBuffer());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particles->getIndexBuffer()->getBuffer());

	glDrawElements(GL_TRIANGLES, GLsizei(particles->getSize() * 6), GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	glDisable(GL_BLEND);

	glutSwapBuffers();
}

void ComputeMain::initRendering(void)
{
	// Read the shader files
	GLuint program = LoadVFShaders("vertex.glsl", "fragment.glsl");
	glUseProgram(program);
	/*
	//create ubo and initialize it with the structure data
	glGenBuffers(1, &mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderParams), &mShaderParams, GL_STREAM_DRAW);*/

	//create simple single-vertex VBO
	float vtx_data[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

	particles = new ParticleSystem(10);

	int cx, cy, cz;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cx);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &cy);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &cz);
	printf("Max compute work group count = %d, %d, %d\n", cx, cy, cz);

	int sx, sy, sz;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sx);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &sy);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &sz);
	printf("Max compute work group size  = %d, %d, %d\n", sx, sy, sz);

	
}

unsigned long getFileLength(ifstream& file)
{
	if (!file.good()) return 0;

	unsigned long pos = file.tellg();
	file.seekg(0, ios::end);
	unsigned long len = file.tellg();
	file.seekg(ios::beg);

	return len;
}

int ComputeMain::readFile(const char* filename, GLchar** ShaderSource, unsigned long* len) {
	ifstream file;
	file.open(filename, ios::in);						// Open the file
	if (!file) return -1;

	*len = getFileLength(file);

	if (len == 0) return -2;							// Error: Empty File 

	*ShaderSource = (GLchar*) new char[*len + 1];
	if (*ShaderSource == 0) return -3;   // can't reserve memory

										 // len isn't always strlen cause some characters are stripped in ascii read...
										 // it is important to 0-terminate the real length later, len is just max possible value... 
	*ShaderSource[*len] = 0;

	unsigned int i = 0;
	while (file.good())
	{
		*ShaderSource[i] = file.get();       // get character from file.
		if (!file.eof())
			i++;
	}

	*ShaderSource[i] = 0;  // 0-terminate it at the correct position

	file.close();

	return 0; // No Error
}


// Main program
int main(int argc, char **argv) {
	// Initialize ComputeMain
	ComputeMain computeMain(argc, argv);

	return 0;
}