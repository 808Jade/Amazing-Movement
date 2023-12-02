#define  _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <gl/glm/ext.hpp>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext/matrix_transform.hpp>
#include <stdlib.h>
#include <random>
#include <fstream>

#define Width 1200
#define Height 800
#define side_length 0.2
#define PI 3.14159265

void make_vertexShaders();
void make_fragmentShaders();
void make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
char* filetobuf(const char* file);
void InitBuffer();

GLvoid Keyboard(unsigned char key, int x, int y);
void move_cube(int);
void rotate_light(int);

GLchar* vertexSource, * fragmentSource; //--- 소스코드 저장 변수
GLuint vertexShader, fragmentShader; //--- 세이더 객체
GLuint shaderProgramID;
GLuint vao;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> random_color(0, 1);
std::uniform_real_distribution<> min_height(0, 5);
std::uniform_real_distribution<> max_height(30, 50);
std::uniform_real_distribution<> speed(0.1, 0.8);

class Shape {
public:
	GLuint vbo[3];
	int vertex_count;
	void load_Obj(const char* path)
	{
		int v_count = 0;
		int n_count = 0;
		int f_count = 0;
		std::string lineHeader;
		std::ifstream in(path);
		if (!in) {
			std::cerr << path << "파일 못찾음";
			exit(1);
		}
		while (in >> lineHeader) {
			if (lineHeader == "v")	++v_count;
			else if (lineHeader == "f")	++f_count;
		}
		in.close();
		in.open(path);

		glm::vec3* vertex = new glm::vec3[v_count];
		glm::vec3* face = new glm::vec3[f_count];
		glm::vec3* vertexdata = new glm::vec3[f_count * 3];
		glm::vec3* normaldata = new glm::vec3[f_count * 3];
		glm::vec3* colordata = new glm::vec3[f_count * 3];
		vertex_count = f_count * 3;
		int v_incount = 0;
		int f_incount = 0;
		int color_count = 0;
		while (in >> lineHeader) {
			if (lineHeader == "v") {
				in >> vertex[v_incount].x >> vertex[v_incount].y >> vertex[v_incount].z;
				++v_incount;
			}
			else if (lineHeader == "f") {
				in >> face[f_incount].x >> face[f_incount].y >> face[f_incount].z;
				vertexdata[f_incount * 3 + 0] = vertex[static_cast<int>(face[f_incount].x - 1)];
				vertexdata[f_incount * 3 + 1] = vertex[static_cast<int>(face[f_incount].y - 1)];
				vertexdata[f_incount * 3 + 2] = vertex[static_cast<int>(face[f_incount].z - 1)];
				++f_incount;
			}
		}
		for (int i = 0; i < vertex_count / 3; i++)
		{
			glm::vec3 normal = glm::cross(vertexdata[i * 3 + 1] - vertexdata[i * 3 + 0], vertexdata[i * 3 + 2] - vertexdata[i * 3 + 0]);
			normaldata[i * 3 + 0] = normal;
			normaldata[i * 3 + 1] = normal;
			normaldata[i * 3 + 2] = normal;
		}
		colordata[0].x = random_color(gen);
		colordata[0].y = random_color(gen);
		colordata[0].z = random_color(gen);
		for (int i = 1; i < vertex_count; ++i) {
			colordata[i].x = colordata[0].x;
			colordata[i].y = colordata[0].y;
			colordata[i].z = colordata[0].z;
		}

		glGenBuffers(3, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		delete[] vertex;
		vertex = nullptr;
		delete[] face;
		face = nullptr;
		delete[] vertexdata;
		vertexdata = nullptr;
		delete[] normaldata;
		normaldata = nullptr;
		delete[] colordata;
		colordata = nullptr;
	}
};

class Cube :public Shape {
public:
	glm::vec3 pos;
	float max_height;
	float min_height;
	float speed;
	float size = 1.f;
	void Set_color() {
		glm::vec3* colordata = new glm::vec3[vertex_count];

		for (int i = 0; i < vertex_count; i += 6) {
			colordata[i + 5].x = colordata[i + 4].x = colordata[i + 3].x = colordata[i + 2].x = colordata[i + 1].x = colordata[i].x = random_color(gen);
			colordata[i + 5].y = colordata[i + 4].y = colordata[i + 3].y = colordata[i + 2].y = colordata[i + 1].y = colordata[i].y = random_color(gen);
			colordata[i + 5].z = colordata[i + 4].z = colordata[i + 3].z = colordata[i + 2].z = colordata[i + 1].z = colordata[i].z = random_color(gen);
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		delete[] colordata;
		colordata = nullptr;
	}
};
class Ground :public Cube {

};

Cube cube[25][25];
Ground ground;
glm::vec3 light_pos;
glm::vec3 light_color;
int length, width;
float light_rail[200][3];

float light_rotate_y;
bool rotate_light_on;
int light_count;

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
{
	while ((length > 25) || (length < 5)) {
		std::cout << "세로 갯수(5~25): ";
		std::cin >> length;
	}
	while ((width > 25) || (width < 5)) {
		std::cout << "가로 갯수(5~25): ";
		std::cin >> width;
	}
	//--- 윈도우 생성하기
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(Width, Height);
	glutCreateWindow("Amazing Movement");
	glewExperimental = GL_TRUE;
	glewInit();
	make_shaderProgram();
	float x;
	float z;
	float angle;
	light_pos.x = 0.f;
	light_pos.y = 20.f;
	light_pos.z = 10.f;
	for (int i = 0; i < 200; ++i) {
		angle = (i + 50) * 1.8f;
		x = cos(angle * PI / 180) * light_pos.z;
		z = sin(angle * PI / 180) * light_pos.z;
		light_rail[i][0] = x;
		light_rail[i][1] = light_pos.y;
		light_rail[i][2] = z;
	}
	light_color.x = 1.f;
	light_color.y = 1.f;
	light_color.z = 1.f;
	InitBuffer();
	std::cout << "1: 애니메이션 1" << std::endl;
	std::cout << "2: 애니메이션 2" << std::endl;
	std::cout << "3: 애니메이션 3" << std::endl;
	std::cout << "t: 조명 on/off" << std::endl;
	std::cout << "c: 조명 색 변경" << std::endl;
	std::cout << "y/Y: 조명 공전" << std::endl;
	std::cout << "+/-: 육면체 이동하는 속도 증가/감소" << std::endl;
	std::cout << "r: 모든 값 초기화" << std::endl;
	std::cout << "q: 프로그램 종료" << std::endl;
	//--- 세이더 읽어와서 세이더 프로그램 만들기
	glutDisplayFunc(drawScene); //--- 출력 콜백 함수
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}

GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수
{
	GLfloat rColor, gColor, bColor;
	rColor = gColor = bColor = 0.2f;
	glClearColor(rColor, gColor, bColor, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	glBindVertexArray(vao);

	int PosLocation = glGetAttribLocation(shaderProgramID, "in_Position"); //	: 0
	int ColorLocation = glGetAttribLocation(shaderProgramID, "in_Color"); //	: 1
	int NormalLocation = glGetAttribLocation(shaderProgramID, "in_Normal"); //	: 2
	glEnableVertexAttribArray(PosLocation);
	glEnableVertexAttribArray(ColorLocation);
	glEnableVertexAttribArray(NormalLocation);

	unsigned int proj_location = glGetUniformLocation(shaderProgramID, "projection");
	unsigned int view_location = glGetUniformLocation(shaderProgramID, "view");
	unsigned int shape_location = glGetUniformLocation(shaderProgramID, "transform");


	//투영행렬
	glm::mat4 proj = glm::mat4(1.0f);
	proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 300.0f);
	proj = glm::translate(proj, glm::vec3(0.f, 0.f, -10.0f));
	glUniformMatrix4fv(proj_location, 1, GL_FALSE, &proj[0][0]);

	//뷰 행렬
	glm::vec3 cameraPos = glm::vec3(-10.f, 40.f, 10.f); //--- 카메라 위치
	glm::vec3 cameraDirection = glm::vec3(0.f, 10.f, 0.f); //--- 카메라 바라보는 방향
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); //--- 카메라 위쪽 방향
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	//view = glm::rotate(view, glm::radians(30.f), glm::vec3(0.f, 1.f, 0.f));
	glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);

	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	glUniform3f(lightPosLocation, light_pos.x, light_pos.y, light_pos.z);
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	glUniform3f(lightColorLocation, light_color.x, light_color.y, light_color.z);
	unsigned int viewPosLocation = glGetUniformLocation(shaderProgramID, "viewPos");
	glUniform3f(viewPosLocation, cameraPos.x, cameraPos.y, cameraPos.z);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, Width, Height);
	glm::mat4 trans = glm::mat4(1.0f);
	trans = glm::translate(trans, glm::vec3(0.f, -1.f, 0.f));
	trans = glm::scale(trans, glm::vec3(300.f, 1.f, 300.f));
	trans = glm::translate(trans, glm::vec3(ground.pos));
	glUniformMatrix4fv(shape_location, 1, GL_FALSE, glm::value_ptr(trans));
	glBindBuffer(GL_ARRAY_BUFFER, ground.vbo[0]);
	glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, ground.vbo[1]);
	glVertexAttribPointer(ColorLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, ground.vbo[2]);
	glVertexAttribPointer(NormalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glDrawArrays(GL_TRIANGLES, 0, ground.vertex_count);

	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < length; ++j) {
			trans = glm::mat4(1.0f);
			trans = glm::translate(trans, glm::vec3(cube[i][j].pos));
			trans = glm::scale(trans, glm::vec3(1.f, cube[i][j].size, 1.f));
			glUniformMatrix4fv(shape_location, 1, GL_FALSE, glm::value_ptr(trans));
			glBindBuffer(GL_ARRAY_BUFFER, cube[i][j].vbo[0]);
			glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
			glBindBuffer(GL_ARRAY_BUFFER, cube[i][j].vbo[1]);
			glVertexAttribPointer(ColorLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
			glBindBuffer(GL_ARRAY_BUFFER, cube[i][j].vbo[2]);
			glVertexAttribPointer(NormalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
			glDrawArrays(GL_TRIANGLES, 0, cube[i][j].vertex_count);

		}
	}

	glViewport(1000, 600, 200, 200);

	cameraPos = glm::vec3(0.f, 22.f, 0.0f);
	cameraDirection = glm::vec3(0.f, 15.f, 0.f);
	cameraUp = glm::vec3(1.0f, 0.0f, 0.0f);
	view = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < length; ++j) {
			trans = glm::mat4(1.0f);
			trans = glm::translate(trans, glm::vec3(cube[i][j].pos));
			trans = glm::scale(trans, glm::vec3(1.f, cube[i][j].size, 1.f));
			glUniformMatrix4fv(shape_location, 1, GL_FALSE, glm::value_ptr(trans));
			glBindBuffer(GL_ARRAY_BUFFER, cube[i][j].vbo[0]);
			glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
			glBindBuffer(GL_ARRAY_BUFFER, cube[i][j].vbo[1]);
			glVertexAttribPointer(ColorLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
			glBindBuffer(GL_ARRAY_BUFFER, cube[i][j].vbo[2]);
			glVertexAttribPointer(NormalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
			glDrawArrays(GL_TRIANGLES, 0, cube[i][j].vertex_count);

		}
	}
	glDisableVertexAttribArray(ColorLocation);
	glDisableVertexAttribArray(PosLocation);
	glDisableVertexAttribArray(NormalLocation);


	glutSwapBuffers(); // 화면에 출력하기
}
GLvoid Reshape(int w, int h) //--- 콜백 함수: 다시 그리기 콜백 함수
{
	glViewport(0, 0, w, h);
}
void make_vertexShaders()
{
	vertexSource = filetobuf("vertex.glsl");
	//--- 버텍스 세이더 객체 만들기
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//--- 세이더 코드를 세이더 객체에 넣기
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	//--- 버텍스 세이더 컴파일하기
	glCompileShader(vertexShader);
	//--- 컴파일이 제대로 되지 않은 경우: 에러 체크
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cout << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}
void make_fragmentShaders()
{
	fragmentSource = filetobuf("fragment.glsl");
	//--- 프래그먼트 세이더 객체 만들기
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//--- 세이더 코드를 세이더 객체에 넣기
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
	//--- 프래그먼트 세이더 컴파일
	glCompileShader(fragmentShader);
	//--- 컴파일이 제대로 되지 않은 경우: 컴파일 에러 체크
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}
void make_shaderProgram()
{
	make_vertexShaders(); //--- 버텍스 세이더 만들기
	make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
	//-- shader Program
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);
	//--- 세이더 삭제하기
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	//--- Shader Program 사용하기
	glUseProgram(shaderProgramID);
}
char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb"); // Open file for reading
	if (!fptr) // Return NULL on failure
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file
	length = ftell(fptr); // Find out how many bytes into the file we are
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer
	fclose(fptr); // Close the file
	buf[length] = 0; // Null terminator
	return buf; // Return the buffer
}
void InitBuffer() {

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	ground.load_Obj("AM_cube.obj");
	ground.pos = { 0.f,-0.4f,0.f };
	ground.Set_color();
	for (int i = 0; i < 25; ++i) {
		for (int j = 0; j < 25; ++j) {
			cube[i][j].load_Obj("AM_cube.obj");
			cube[i][j].Set_color();
		}
	}
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < length; ++j) {
			cube[i][j].pos.x = -4.8 + (0.4 * i);
			cube[i][j].pos.z = -4.8 + (0.4 * j);
			cube[i][j].speed = speed(gen);
			cube[i][j].max_height = max_height(gen);
			cube[i][j].min_height = min_height(gen);
		}
	}
	glutTimerFunc(15, move_cube, 0);
}
GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case '1':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].speed = speed(gen);
				cube[i][j].max_height = max_height(gen);
				cube[i][j].min_height = min_height(gen);
			}
		}
		break;
	case '2':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].size = i * 1.f;
				cube[i][j].speed = 1.f;
				cube[i][j].max_height = 50.f;
				cube[i][j].min_height = 0.f;
			}
		}
		break;
	case '3':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].size = 45.f;
				cube[i][j].speed = 0.5f;
				cube[i][j].max_height = 50.f;
				cube[i][j].min_height = 0.f;
			}
		}
		for (int i = 0; i < width; ++i) {
			for (int j = i; j < length; ++j) {
				cube[i][j].size -= i * 3.f;
			}
		}
		for (int i = width; i > 0; --i) {
			for (int j = 0; j < i; ++j) {
				if (cube[i][j].size == 45.f)
					cube[i][j].size -= (width - i) * 3.f;
			}
		}
		break;
	case 'c':
		if (light_color.x == 1.0f) {
			light_color = { 0.f,1.f,0.f };
		}
		else if (light_color.y == 1.0f) {
			light_color = { 0.f,0.f,1.f };
		}
		else if (light_color.z == 1.0f) {
			light_color = { 1.f,0.f,0.f };
		}
		break;
	case 't':
	case 'T':
		if (light_color.x == 0.15f) {
			light_color = { 1.f,1.f,1.f };
		}
		else {
			light_color = { 0.15f,0.15f,0.15f };

		}
		break;
	case '+':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				if (cube[i][j].speed > 0) {
					cube[i][j].speed += 0.1f;
				}
				else {
					cube[i][j].speed -= 0.1f;

				}
			}
		}
		break;
	case '-':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				if (cube[i][j].speed > 0) {
					cube[i][j].speed -= 0.1f;
				}
				else {
					cube[i][j].speed += 0.1f;

				}
			}
		}
		break;
	case 'r':
		length = 0;
		width = 0;
		while ((length > 25) || (length < 5)) {
			std::cout << "세로 갯수(5~25): ";
			std::cin >> length;
		}
		while ((width > 25) || (width < 5)) {
			std::cout << "가로 갯수(5~25): ";
			std::cin >> width;
		}
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].load_Obj("AM_cube.obj");
				cube[i][j].Set_color();
				cube[i][j].pos.x = -4.8 + (0.4 * i);
				cube[i][j].pos.z = -4.8 + (0.4 * j);
				cube[i][j].speed = speed(gen);
				cube[i][j].max_height = max_height(gen);
				cube[i][j].min_height = min_height(gen);
			}
		}
		light_color = { 1.f,1.f,1.f };
		break;
	case 'y':
		if (!rotate_light_on)
		{
			rotate_light_on = true;
			glutTimerFunc(50, rotate_light, 0);
		}
		else
			rotate_light_on = false;
		break;
	case 'Y':
		if (!rotate_light_on)
		{
			rotate_light_on = true;
			glutTimerFunc(50, rotate_light, 1);
		}
		else
			rotate_light_on = false;
		break;
	case 'q':
	case 'Q':
		exit(0);
		break;
	}
	glutPostRedisplay();
}

void move_cube(int value) {
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < length; ++j) {
			cube[i][j].size += cube[i][j].speed;
			if (cube[i][j].speed > 0) {
				if (cube[i][j].size >= cube[i][j].max_height) {
					cube[i][j].speed *= -1;
				}
			}
			else {
				if (cube[i][j].size <= cube[i][j].min_height) {
					cube[i][j].speed *= -1;
				}
			}
		}
	}
	glutTimerFunc(15, move_cube, value);
	glutPostRedisplay();
}

void rotate_light(int value)
{
	light_pos.x = light_rail[light_count][0];
	light_pos.y = light_rail[light_count][1];
	light_pos.z = light_rail[light_count][2];
	if (value == 0) {
		light_count++;
		light_rotate_y -= 1.8f;
	}
	else {
		light_count--;
		light_rotate_y -= 1.8f;
	}
	if (light_count >= 200)
	{
		light_count = 0;
		light_rotate_y = 0;
	}
	else if (light_count <= 0) {
		light_count = 200;
		light_rotate_y = 0;
	}
	if (rotate_light_on)
		glutTimerFunc(1, rotate_light, value);
	glutPostRedisplay();
}