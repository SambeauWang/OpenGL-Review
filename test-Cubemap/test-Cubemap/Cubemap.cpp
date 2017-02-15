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

GLTriangleBatch torusBatch;
GLBatch cubeBatch;
GLuint flatShader;
GLuint skyBoxShader;
GLint locMVP;
GLint locMV;
GLint locNM;
GLMatrixStack modelViewMatrix;
GLMatrixStack projectionMatrix;
GLGeometryTransform transformPipleline;
GLuint texture;
GLuint tarnishMap;
GLuint loctarnishMap;
GLuint locTexture;
GLint locInvertedCamera;
GLint locMVPSkyBox;

const char *szCubeFaces[6] = { "pos_x.tga", "neg_x.tga", "pos_y.tga", "neg_y.tga", "pos_z.tga", "neg_z.tga" };

GLenum  cube[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

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

	GLbyte *pBits;
	int nWidth, nHeight, nCompents;
	GLenum eFormat;
	int i;

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (i = 0; i < 6; i++){
		pBits = gltReadTGABits(szCubeFaces[i], &nWidth, &nHeight, &nCompents, &eFormat);
		glTexImage2D(cube[i], 0, nCompents, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
		free(pBits);
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glGenTextures(1, &tarnishMap);
	glBindTexture(GL_TEXTURE_2D, tarnishMap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	pBits = gltReadTGABits("tarnish.tga", &nWidth, &nHeight, &nCompents, &eFormat);
	glTexImage2D(GL_TEXTURE_2D, 0, nCompents, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
	free(pBits);
	glGenerateMipmap(GL_TEXTURE_2D);

	viewFrame.MoveForward(4.0f);
	gltMakeSphere(torusBatch, 1.0f, 52, 26);
	gltMakeCube(cubeBatch, 20.0f);

	flatShader = gltLoadShaderPairWithAttributes("Reflection.vp", "Reflection.fp", 3,
							GLT_ATTRIBUTE_VERTEX, "vVertex",
							GLT_ATTRIBUTE_NORMAL, "vNormal",
							GLT_ATTRIBUTE_TEXTURE0, "vTexCoords");

	locMVP = glGetUniformLocation(flatShader, "mvpMatrix");
	locMV = glGetUniformLocation(flatShader, "mvMatrix");
	locNM = glGetUniformLocation(flatShader, "normalMatrix");
	locInvertedCamera = glGetUniformLocation(flatShader, "mInverseCamera");
	loctarnishMap = glGetUniformLocation(flatShader, "tarnishMap");
	locTexture = glGetUniformLocation(flatShader, "cubeMap");

	skyBoxShader = gltLoadShaderPairWithAttributes("SkyBox.vp", "SkyBox.fp", 2,
		GLT_ATTRIBUTE_VERTEX, "vVertex",
		GLT_ATTRIBUTE_NORMAL, "vNormal");
	locMVPSkyBox = glGetUniformLocation(skyBoxShader, "mvpMatrix");
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tarnishMap);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
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

void SpecialKeys(int key, int x, int y)
{
	if (key == GLUT_KEY_UP)
		viewFrame.MoveForward(0.1f);

	if (key == GLUT_KEY_DOWN)
		viewFrame.MoveForward(-0.1f);

	if (key == GLUT_KEY_LEFT)
		viewFrame.RotateLocalY(0.1);

	if (key == GLUT_KEY_RIGHT)
		viewFrame.RotateLocalY(-0.1);

	// Refresh the Window
	glutPostRedisplay();
}

void RenderScene(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	M3DMatrix44f mCamera;
	M3DMatrix44f mCameraRotOnly;
	M3DMatrix44f InvertedCamera;
	viewFrame.GetCameraMatrix(mCamera, false);
	viewFrame.GetCameraMatrix(mCameraRotOnly, true);
	m3dInvertMatrix44(InvertedCamera, mCameraRotOnly);

	modelViewMatrix.PushMatrix();
		modelViewMatrix.MultMatrix(mCamera);

		glUseProgram(flatShader);

		glUniformMatrix4fv(locMV, 1, GL_FALSE, transformPipleline.GetModelViewMatrix());
		glUniformMatrix4fv(locMVP, 1, GL_FALSE, transformPipleline.GetModelViewProjectionMatrix());
		glUniformMatrix3fv(locNM, 1, GL_FALSE, transformPipleline.GetNormalMatrix());
		glUniformMatrix4fv(locInvertedCamera, 1, GL_FALSE, InvertedCamera);
		glUniform1i(locTexture, 0);
		glUniform1i(loctarnishMap, 1);

		glEnable(GL_CULL_FACE);
		torusBatch.Draw();
		glDisable(GL_CULL_FACE);
	modelViewMatrix.PopMatrix();

	modelViewMatrix.PushMatrix();
		modelViewMatrix.MultMatrix(mCameraRotOnly);
		glUseProgram(skyBoxShader);
		glUniformMatrix4fv(locMVPSkyBox, 1, GL_FALSE, transformPipleline.GetModelViewProjectionMatrix());

		cubeBatch.Draw();
	modelViewMatrix.PopMatrix();

	glutSwapBuffers();
}

int main(int argc, char* argv[]){
	gltSetWorkingDirectory(argv[0]);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Simple CubeMap");
	glutReshapeFunc(ChangeSize);
	glutDisplayFunc(RenderScene);
	glutSpecialFunc(SpecialKeys);

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