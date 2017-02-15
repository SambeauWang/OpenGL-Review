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

#define NUM_SPHERES 50

GLBatch floorBatch;
GLTriangleBatch torusBatch;
GLTriangleBatch sphereBatch;
GLFrame spheres[NUM_SPHERES];

GLShaderManager shaderManager;
GLFrustum viewFrustum;
GLMatrixStack modelviewMatrix;
GLMatrixStack projectMatrix;
GLGeometryTransform transformPipeline;
GLFrame camesFrame;

void ChangeSize(int w, int h){
	if (h == 0)
		h = 1;

	glViewport(0, 0, w, h);
	viewFrustum.SetPerspective(35.0f, float(w) / float(h), 1.0f, 1000.0f);
	projectMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());

	transformPipeline.SetMatrixStacks(modelviewMatrix, projectMatrix);
}


void SetupRC(){
	// glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	/*glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	shaderManager.InitializeStockShaders();
	glEnable(GL_DEPTH_TEST);

	gltMakeTorus(torusBatch, 0.4f, 0.15f, 30, 30);
	gltMakeSphere(sphereBatch, 0.1f, 26, 13);

	floorBatch.Begin(GL_LINES, 324);
	for (GLfloat x = -20.0; x <= 20.0f; x += 0.5) {
		floorBatch.Vertex3f(x, -0.55f, 20.0f);
		floorBatch.Vertex3f(x, -0.55f, -20.0f);

		floorBatch.Vertex3f(20.0f, -0.55f, x);
		floorBatch.Vertex3f(-20.0f, -0.55f, x);
	}
	floorBatch.End();

	for (int i = 0; i < NUM_SPHERES; i++){
		GLfloat x = ((GLfloat)(rand() % 400) - 200) *0.1f;
		GLfloat z = ((GLfloat)(rand() % 400) - 200) *0.1f;
		spheres[i].SetOrigin(x,0.0f,z);
	}
}

void RenderScene(void){
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//shaderManager.UseStockShader(GLT_SHADER_IDENTITY,vRed);
	//traingleBatch.Draw();
	GLfloat vBlock[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	static GLfloat vfloorColor[] = {0.0f,1.0f,0.0f,1.0f};
	static GLfloat vtorusColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	static GLfloat vSphereColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

	static CStopWatch rotTimer;
	float yRot = rotTimer.GetElapsedSeconds()*60.0;

	M3DMatrix44f cameraMatrix;
	camesFrame.GetCameraMatrix(cameraMatrix);
	modelviewMatrix.PushMatrix(cameraMatrix);

	M3DVector4f vLightPos = { 0.0f, 10.0f, 5.0f, 1.0f };
	M3DVector4f vLighteyePos;
	m3dTransformVector4(vLighteyePos, vLightPos, cameraMatrix);
	/*shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(),
		transformPipeline.GetProjectionMatrix(), vLighteyePos, vSphereColor);*/

	modelviewMatrix.PushMatrix();
	shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vfloorColor);
	floorBatch.Draw();

	for (int i = 0; i < NUM_SPHERES; i++){
		modelviewMatrix.PushMatrix();
		modelviewMatrix.MultMatrix(spheres[i]);
		/*shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vSphereColor);*/
		shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(),
			transformPipeline.GetProjectionMatrix(), vLighteyePos, vSphereColor);
		sphereBatch.Draw();
		modelviewMatrix.PopMatrix();
	}

	modelviewMatrix.Translate(0.0f, 0.0f, -2.5f);
	modelviewMatrix.PushMatrix();
	modelviewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);

	/*shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vtorusColor);*/
	shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(),
		transformPipeline.GetProjectionMatrix(), vLighteyePos, vtorusColor);
	torusBatch.Draw();

	modelviewMatrix.PopMatrix();
	modelviewMatrix.Rotate(-2.0 * yRot, 0.0f, 1.0f, 0.0f);
	modelviewMatrix.Translate(0.8f, 0.0f, 0.0f);
	/*shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vSphereColor);*/
	shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(),
		transformPipeline.GetProjectionMatrix(), vLighteyePos, vSphereColor);
	sphereBatch.Draw();

	modelviewMatrix.PopMatrix();
	modelviewMatrix.PopMatrix();

	//M3DMatrix44f finalMatrix, mTransMatrix, mRotationMatrix, mModelViewProjection;

	//m3dTranslationMatrix44(mTransMatrix, blockX, blockY, -2.5f);

	///*static float yRot = 0.0f;
	//yRot += 5.0f;*/
	//static CStopWatch rotTimer;
	//float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
	//m3dRotationMatrix44(mRotationMatrix, m3dDegToRad(yRot), 1.0f, 1.0f, 0.0f);

	//m3dMatrixMultiply44(finalMatrix, mTransMatrix, mRotationMatrix);
	//m3dMatrixMultiply44(mModelViewProjection, viewFrustum.GetProjectionMatrix(), finalMatrix);

	//glEnable(GL_LINE_SMOOTH);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_POLYGON_OFFSET_LINE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glLineWidth(2.5f);

	//shaderManager.UseStockShader(GLT_SHADER_FLAT, mModelViewProjection, vBlock);
	//mySphereBatch.Draw();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_POLYGON_OFFSET_LINE);
	//glLineWidth(1.0f);
	//glDisable(GL_BLEND);
	//glDisable(GL_LINE_SMOOTH);

	glutSwapBuffers();
	glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y){
	GLfloat stepSize = 0.1f;
	float angelRotation = float(m3dDegToRad(5.0f));

	if (key == GLUT_KEY_DOWN)
		camesFrame.MoveForward(-stepSize);
	if (key == GLUT_KEY_UP)
		camesFrame.MoveForward(stepSize);
	if (key == GLUT_KEY_LEFT)
		camesFrame.RotateWorld(angelRotation,0.0f,1.0f,0.0f);
	if (key == GLUT_KEY_RIGHT)
		camesFrame.RotateWorld(-angelRotation,0.0f,1.0f,0.0f);
	glutPostRedisplay();
}

int main(int argc, char* argv[]){
	gltSetWorkingDirectory(argv[0]);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
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

