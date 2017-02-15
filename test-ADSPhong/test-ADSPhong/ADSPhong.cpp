#include <GLTools.h>	// OpenGL toolkit
#include <GLMatrixStack.h>
#include <GLFrame.h>
#include <GLFrustum.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>

#include <math.h>
#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif


GLFrame viewFrame;
GLFrustum viewFrustum;
GLShaderManager shaderManager;

GLTriangleBatch torusBatch;
GLuint flatShader;
GLint locMVP;
GLint locMV;
//GLint locColor;
GLint locAmbient;
GLint locDiffuse;
GLint locSpecular;
GLint locLight;
GLint locNM;
GLMatrixStack modelViewMatrix;
GLMatrixStack projectionMatrix;
GLGeometryTransform transformPipleline;
GLuint texture;
GLint locTexture;

bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode){
	GLbyte *pBits;
	int nWidth, nHeight, nCompents;
	GLenum eFormat;

	pBits = gltReadTGABits(szFileName, &nWidth, &nHeight, &nCompents, &eFormat);
	if (NULL == pBits)
		return false;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, nCompents, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
	free(pBits);


	if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
		minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		minFilter == GL_NEAREST_MIPMAP_LINEAR ||
		minFilter == GL_NEAREST_MIPMAP_NEAREST)
		glGenerateMipmap(GL_TEXTURE_2D);

	return true;
}

void SetupRC(void){
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glEnable(GL_DEPTH_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	shaderManager.InitializeStockShaders();

	viewFrame.MoveForward(4.0f);
	gltMakeSphere(torusBatch, 1.0f, 26, 13);
	//gltMakeTorus(torusBatch, 0.80f, 0.25f, 52, 26);
	flatShader = gltLoadShaderPairWithAttributes("ADSPhong.vp", "ADSPhong.fp", 3, GLT_ATTRIBUTE_VERTEX, "vVertex",
		GLT_ATTRIBUTE_NORMAL, "vNormal", GLT_ATTRIBUTE_TEXTURE0, "vTexture0");

	locMVP = glGetUniformLocation(flatShader, "mvpMatrix");
	locMV = glGetUniformLocation(flatShader, "mvMatrix");
	//locColor = glGetUniformLocation(flatShader, "vColorValue");
	locAmbient = glGetUniformLocation(flatShader, "ambientColor");
	locDiffuse = glGetUniformLocation(flatShader, "diffuseColor");
	locSpecular = glGetUniformLocation(flatShader, "specularColor");
	locLight = glGetUniformLocation(flatShader, "vLightPosition");
	locNM = glGetUniformLocation(flatShader, "normalMatrix");
	locTexture = glGetUniformLocation(flatShader, "colorMap");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	LoadTGATexture("CoolTexture.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

}

void ShutdownRC(void){
	glDeleteTextures(1, &texture);
}

void ChangeSize(int w, int h){
	if (h == 0)
		h = 1;
	glViewport(0, 0, w, h);

	viewFrustum.SetPerspective(35.0f, float(w) / float(h), 1.0f, 100.0f);
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
	transformPipleline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

void RenderScene(){
	static CStopWatch rotTimer;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//GLfloat vColor[] = { 0.1f, 0.1f, 1.f, 1.0f };

	modelViewMatrix.PushMatrix(viewFrame);
		modelViewMatrix.Rotate(rotTimer.GetElapsedSeconds() * 10.0f, 0.0f, 1.0f, 0.0f);
		GLfloat vEyeLight[] = { -100.0f, 100.0f, 100.0f };
		GLfloat vAmbientColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat vDiffuseColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
		GLfloat vSpecularColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		glBindTexture(GL_TEXTURE_2D, texture);
		glUseProgram(flatShader);
		//glUniform4fv(locColor, 1, vColor);
		glUniform4fv(locAmbient, 1, vAmbientColor);
		glUniform4fv(locDiffuse, 1, vDiffuseColor);
		glUniform4fv(locSpecular, 1, vSpecularColor);
		glUniform3fv(locLight, 1, vEyeLight);
		glUniformMatrix4fv(locMV, 1, GL_FALSE, transformPipleline.GetModelViewMatrix());
		glUniformMatrix4fv(locMVP, 1, GL_FALSE, transformPipleline.GetModelViewProjectionMatrix());
		glUniformMatrix3fv(locNM, 1, GL_FALSE, transformPipleline.GetNormalMatrix());
		glUniform1i(locTexture,0);
		torusBatch.Draw();
	modelViewMatrix.PopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}

int main(int argc, char* argv[]){
	gltSetWorkingDirectory(argv[0]);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Simple ADA Phong");
	glutReshapeFunc(ChangeSize);
	glutDisplayFunc(RenderScene);

	GLenum err = glewInit();
	if (GLEW_OK != err){
		fprintf(stderr, "GLEW ERROR: %s", glewGetErrorString(err));
		return 1;
	}

	SetupRC();
	glutMainLoop();
	ShutdownRC();
	return 0;
}