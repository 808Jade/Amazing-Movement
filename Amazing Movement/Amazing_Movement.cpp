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

#define PI 3.141592

void make_vertexShaders();
void make_fragmentShaders();
void make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
char* filetobuf(const char* file);

GLvoid Keyboard(unsigned char key, int x, int y);
void LoadObj(const char* path);
void InitBuffer();
void MoveCube(int value);

GLchar* vertexSource, *fragmentSource; //--- 소스코드 저장 변수
GLuint vertexShader, fragmentShader; //--- 세이더 객체
GLuint shaderProgramID;
GLuint vao;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> random_color(0, 1);
std::uniform_real_distribution<> speed(0.2, 0.6);
std::uniform_real_distribution<> height_max(0, 5);
std::uniform_real_distribution<> height_min(30, 50);

class Cube {
public:
	GLuint vbo[3];
	glm::vec3 pos;
	float height_max;
	float height_min;
	float speed;
	float size = 1.f;

	int vertex_count = 36;

	void LoadObj(const char* path) {
		std::vector<int> vertexIndices, uvIndices, normalIndices;
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> uvs;

		std::ifstream in(path);
		if (!in) {
			std::cerr << path << " 파일 못찾음";
			exit(1);
		}

		std::string lineHeader;
		while (in >> lineHeader) {
			if (lineHeader == "v") {
				glm::vec3 vertex;
				in >> vertex.x >> vertex.y >> vertex.z;
				vertices.push_back(vertex);
			}
			else if (lineHeader == "vt") {
				glm::vec2 uv;
				in >> uv.x >> uv.y;
				uvs.push_back(uv);
			}
			else if (lineHeader == "vn") {
				glm::vec3 normal;
				in >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			}
			else if (lineHeader == "f") {
				char a;
				int vertexIndex[3], uvIndex[3], normalIndex[3];

				for (int i = 0; i < 3; i++) {
					in >> vertexIndex[i] >> a >> uvIndex[i] >> a >> normalIndex[i];
					vertexIndices.push_back(vertexIndex[i] - 1);
					uvIndices.push_back(uvIndex[i] - 1);
					normalIndices.push_back(normalIndex[i] - 1);
				}
			}
		}

		std::vector<glm::vec3> colors(vertices.size());

		colors[0].x = random_color(gen);
		colors[0].y = random_color(gen);
		colors[0].z = random_color(gen);

		for (size_t i = 1; i < vertices.size(); ++i) {
			colors[i] = colors[0];
		}

		// 그냥 바인딩까지
		glGenBuffers(3, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}
};

Cube cube[25][25];

glm::vec3 light_pos;
glm::vec3 light_color;
float light_rotate_y;
const int numPoints = 200;
bool light_flag;
std::vector<glm::vec3> light_rail(numPoints);
int light_count;

int length{};
int width{};
void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
{
	while (width < 5 || width > 25 || length < 5 || length > 25) {
		std::cout << "가로 세로 입력 ( 5 ~ 25 ) : ";
		std::cin >> length >> width;

		if (width < 5 || width > 25 || length < 5 || length > 25) {
			std::cout << "입력 값이 범위를 벗어납니다." << '\n';
		}
	}
	system("cls");
	std::cout << "1   : 애니메이션 1" << '\n';
	std::cout << "2   : 애니메이션 2" << '\n';
	std::cout << "3   : 애니메이션 3" << '\n';
	std::cout << "t   : 조명을 켠다/끈다." << '\n';
	std::cout << "c   : 조명 색을 바꾼다." << '\n';
	std::cout << "+/- : 육면체 이동하는 속도 증가/감소" << '\n';
	std::cout << "r   : 모든 값 초기화" << '\n';
	std::cout << "q   : 프로그램 종료" << '\n';

	//--- 윈도우 생성하기
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(300, 100);
	glutInitWindowSize(1300, 800);
	glutCreateWindow("Amazing Movement");
	glewExperimental = GL_TRUE;
	glewInit();
	make_shaderProgram();


	// 초기화
	light_pos = glm::vec3(0.f, 20.f, 10.f);
	light_color = glm::vec3(1.f, 1.f, 1.f);
	// 빛의 경로 계산
	for (int i = 0; i < numPoints; ++i) {
		float angle = (i + 50) * 1.8f;
		float x = cos(angle * PI / 180) * light_pos.z;
		float z = sin(angle * PI / 180) * light_pos.z;

		light_rail[i] = glm::vec3(x, light_pos.y, z);
	}

	InitBuffer();
	glutTimerFunc(15, MoveCube, 0);
	//--- 세이더 읽어와서 세이더 프로그램 만들기
	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}

GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	glBindVertexArray(vao);

	int PosLocation = glGetAttribLocation(shaderProgramID, "in_Position");	 // : 0
	int ColorLocation = glGetAttribLocation(shaderProgramID, "in_Color");    // : 1
	int NormalLocation = glGetAttribLocation(shaderProgramID, "in_Normal");  // : 2
	glEnableVertexAttribArray(PosLocation);
	glEnableVertexAttribArray(ColorLocation);
	glEnableVertexAttribArray(NormalLocation);

	// 투영 행렬 설정
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 300.0f);
	projection = glm::translate(projection, glm::vec3(0.f, 0.f, -10.0f));
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projection");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	// 뷰 행렬 설정
	glm::vec3 cameraPos = glm::vec3(5.f, 30.f, 10.f);
	glm::vec3 cameraDirection = glm::vec3(-1.f, 3.f, 0.f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	unsigned int view_location = glGetUniformLocation(shaderProgramID, "view");
	glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);

	// 광원 및 시야 설정
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	unsigned int viewPosLocation = glGetUniformLocation(shaderProgramID, "viewPos");

	glUniform3f(glGetUniformLocation(shaderProgramID, "lightPos"), light_pos.x, light_pos.y, light_pos.z);
	glUniform3f(glGetUniformLocation(shaderProgramID, "lightColor"), light_color.x, light_color.y, light_color.z);
	glUniform3f(glGetUniformLocation(shaderProgramID, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);



	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1300, 800);
	glm::mat4 trans = glm::mat4(1.0f);
	trans = glm::translate(trans, glm::vec3(0.f, -1.f, 0.f));
	trans = glm::scale(trans, glm::vec3(300.f, 1.f, 300.f));
	unsigned int shape_location = glGetUniformLocation(shaderProgramID, "transform");


	// 각각의 큐브를 그립니다.
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

	// 미니맵
	glViewport(1000, 600, 300, 300);

	cameraPos = glm::vec3(0.f, 23.f, 0.0f);
	cameraDirection = glm::vec3(0.f, 0.f, 0.f);
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

	glutSwapBuffers();
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

	for (int i = 0; i < 25; ++i) {
		for (int j = 0; j < 25; ++j) {
			cube[i][j].LoadObj("cube.obj");
		}
	}
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < length; ++j) {
			cube[i][j].pos.x = -5.8 + (0.8 * i);	// 수정
			cube[i][j].pos.z = -5.8 + (0.8 * j);	// 수정
			cube[i][j].speed = speed(gen);
			cube[i][j].height_max = height_max(gen);
			cube[i][j].height_min = height_min(gen);
		}
	}
}

void MoveCube(int value) {
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < length; ++j) {
			cube[i][j].size += cube[i][j].speed;
			if (cube[i][j].speed > 0) {
				if (cube[i][j].size >= cube[i][j].height_max) {
					cube[i][j].speed = -cube[i][j].speed;
				}
			}
			else {
				if (cube[i][j].size <= cube[i][j].height_min) {
					cube[i][j].speed = -cube[i][j].speed;
				}
			}
		}
	}
	glutTimerFunc(15, MoveCube, value);
	glutPostRedisplay();
}

//void MoveCube3(int value) {
//	if (value == 0) {
//		light_color.x = random_color(gen);
//		light_color.y = random_color(gen);
//		light_color.z = random_color(gen);
//	}
//	else if (value == 1) {
//		light_color = { 1.f,1.f,1.f };
//	}
//	glutTimerFunc(15, MoveCube3, value);
//	glutPostRedisplay();
//}

bool rotate_3_on = false;
GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case '1':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].size = 20.f;
				cube[i][j].speed = speed(gen);
				cube[i][j].height_max = 40.f;
				cube[i][j].height_min = 10.f;

				// 추가된 부분: 특별한 패턴을 생성하여 크기 및 높이를 조절
				float patternSize = 5.f;  // 패턴의 크기
				float patternHeight = 10.f;  // 패턴의 높이

				// 특별한 패턴 적용
				if (i % static_cast<int>(patternSize) == 0 && j % static_cast<int>(patternSize) == 0) {
					cube[i][j].size += patternSize;
					cube[i][j].height_max += patternHeight;
					cube[i][j].height_min += patternHeight / 2.f;
				}
			}
		}
		break;
	case '2':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].size = i * 1.f;
				cube[i][j].speed = 1.f;
				cube[i][j].height_max = 30.f;
				cube[i][j].height_min = 0.f;
			}
		}
		break;
	case '3':
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				// 크기와 높이를 물결 모양으로 설정
				cube[i][j].size = 15.f + 5.f * sin(i * 0.1f);  // 사인 함수를 사용하여 크기를 물결 모양으로 조절
				cube[i][j].speed = 0.5f;  // 고정된 속도
				cube[i][j].height_max = 30.f + 10.f * sin(j * 0.1f);  // 사인 함수를 사용하여 최대 높이를 물결 모양으로 조절
				cube[i][j].height_min = 5.f;  // 최소 높이를 고정값으로 설정
				/*if (!rotate_3_on) {
					rotate_3_on = true;
					glutTimerFunc(15, MoveCube3, 0);
				}
				else {
					rotate_3_on = false;
					glutTimerFunc(15, MoveCube3, 1);
				}*/
			}
		}
		break;
	case 'c':
		light_color.x = random_color(gen);
		light_color.y = random_color(gen);
		light_color.z = random_color(gen);
		break;
	case 't':
	case 'T':
		if (light_color.x == 0.1f) {
			light_color = { 1.f,1.f,1.f };
		}
		else {
			light_color = { 0.1f,0.1f,0.1f };
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
		while (width < 5 || width > 25 || length < 5 || length > 25) {
			std::cout << "가로 세로 입력 ( 5 ~ 25 ) : ";
			std::cin >> width >> length;

			if (width < 5 || width > 25 || length < 5 || length > 25) {
				std::cout << "입력 값이 범위를 벗어납니다." << '\n';
			}
		}
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < length; ++j) {
				cube[i][j].size = 20.f;
				cube[i][j].speed = speed(gen);
				cube[i][j].height_max = 40.f;
				cube[i][j].height_min = 10.f;

				// 추가된 부분: 특별한 패턴을 생성하여 크기 및 높이를 조절
				float patternSize = 5.f;  // 패턴의 크기
				float patternHeight = 10.f;  // 패턴의 높이

				// 특별한 패턴 적용
				if (i % static_cast<int>(patternSize) == 0 && j % static_cast<int>(patternSize) == 0) {
					cube[i][j].size += patternSize;
					cube[i][j].height_max += patternHeight;
					cube[i][j].height_min += patternHeight / 2.f;
				}
			}
		}
		light_color = { 1.f,1.f,1.f };
		break;
	case 'q':
	case 'Q':
		exit(0);
		break;
	}
	glutPostRedisplay();
}