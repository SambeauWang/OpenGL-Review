#include <GLTools.h>
#include <GLShaderManager.h>
#include <GLMatrixStack.h>
#include <GLFrame.h>
#include <GLFrustum.h>
#include <GLBatch.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>

#include <math.h>
#define FREEGLUT_STATIC
#include<GL/glut.h>

GLfloat blockX=0;
GLfloat blockY=0;

GLBatch traingleBatch;
GLBatch	triangleStripBatch;
GLTriangleBatch mySphereBatch;
GLShaderManager shaderManager;
GLFrustum viewFrustum;


void ChangeSize(int w, int h){
	if (h == 0)
		h = 1;

	glViewport(0, 0, w, h);
	viewFrustum.SetPerspective(35.0f,float(w)/float(h),1.0f,1000.0f);
}


void SetupRC(){
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	shaderManager.InitializeStockShaders();
	glEnable(GL_DEPTH_TEST);

	gltMakeTorus(mySphereBatch, 0.3f, 0.1f, 20, 10);
}

void RenderScene(void){
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//shaderManager.UseStockShader(GLT_SHADER_IDENTITY,vRed);
	//traingleBatch.Draw();
	GLfloat vBlock[] = {0.0f, 0.0f, 0.0f, 1.0f};
	M3DMatrix44f finalMatrix, mTransMatrix, mRotationMatrix, mModelViewProjection;

	m3dTranslationMatrix44(mTransMatrix,blockX,blockY,-2.5f);
	
	/*static float yRot = 0.0f;
	yRot += 5.0f;*/
	static CStopWatch rotTimer;
	float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
	m3dRotationMatrix44(mRotationMatrix, m3dDegToRad(yRot), 1.0f, 1.0f, 0.0f);

	m3dMatrixMultiply44(finalMatrix,mTransMatrix,mRotationMatrix);
	m3dMatrixMultiply44(mModelViewProjection, viewFrustum.GetProjectionMatrix(), finalMatrix);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(2.5f);

	shaderManager.UseStockShader(GLT_SHADER_FLAT, mModelViewProjection, vBlock);
	mySphereBatch.Draw();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glLineWidth(1.0f);
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);

	glutSwapBuffers();
	glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y){
	GLfloat stepSize = 0.025f;

	if (key == GLUT_KEY_DOWN)
		blockY -= stepSize;
	if (key == GLUT_KEY_UP)
		blockY += stepSize;
	if (key == GLUT_KEY_LEFT)
		blockX -= stepSize;
	if (key == GLUT_KEY_RIGHT)
		blockX += stepSize;

	glutPostRedisplay();
}

int main(int argc, char* argv[]){
	gltSetWorkingDirectory(argv[0]);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(800, 600);
	glutCreateWindow("triangle");
	glutReshapeFunc(ChangeSize);
	glutDisplayFunc(RenderScene);
	glutSpecialFunc(SpecialKeys);

	GLenum err = glewInit();
	if (GLEW_OK != err){
		fprintf(stderr, "error!");
		return 1;
	}

	SetupRC();

	glutMainLoop();
	return 0;
}

