#include <stdio.h>
#include <iostream>

#include <GLTools.h>
#include <GLShaderManager.h>
#include <GLFrustum.h>
#include <GLBatch.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>

#include <GL/glu.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

static GLfloat vGreen[] = { 0.0f, 1.0f, 0.0f, 1.0f };
static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat vLightPos[] = { 0.0f, 3.0f, 0.0f, 1.0f };

static const GLenum windowBuff[] = { GL_BACK_LEFT };
static const GLenum fboBuffs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
GLenum curBuffs = GL_COLOR_ATTACHMENT1;

GLsizei	 screenWidth;			// Desired window or desktop width
GLsizei  screenHeight;			// Desired window or desktop height

GLboolean bFullScreen;			// Request to run full screen
GLboolean bAnimated;			// Request for continual updates


GLShaderManager		shaderManager;			// Shader Manager
GLMatrixStack		modelViewMatrix;		// Modelview Matrix
GLMatrixStack		projectionMatrix;		// Projection Matrix
M3DMatrix44f        orthoMatrix;
GLFrustum			viewFrustum;			// View Frustum
GLGeometryTransform	transformPipeline;		// Geometry Transform Pipeline
GLFrame				cameraFrame;			// Camera frame

GLTriangleBatch		torusBatch;
GLBatch				floorBatch;
GLBatch             screenQuad;

GLuint				textures[1];
GLuint				blurTextures[6];
GLuint				pixBuffObjs[1];
GLuint				curBlurTarget;
bool				bUsePBOPath;
GLfloat				speedFactor;
GLuint				blurProg;
void				*pixelData;
GLuint				pixelDataSize;

GLuint              fboName;
GLuint              depthBufferName;
GLuint				renderBufferNames[3];

void MoveCamera(void);
void DrawWorld(GLfloat yRot, GLfloat xPos);
bool LoadBMPTexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode);

void SetupBlurProg(void);

// returns 1 - 6 for blur texture units
// curPixBuf is always between 0 and 5
void AdvanceBlurTaget() { curBlurTarget = ((curBlurTarget + 1) % 6); }
GLuint GetBlurTarget0(){ return (1 + ((curBlurTarget + 5) % 6)); }
GLuint GetBlurTarget1(){ return (1 + ((curBlurTarget + 4) % 6)); }
GLuint GetBlurTarget2(){ return (1 + ((curBlurTarget + 3) % 6)); }
GLuint GetBlurTarget3(){ return (1 + ((curBlurTarget + 2) % 6)); }
GLuint GetBlurTarget4(){ return (1 + ((curBlurTarget + 1) % 6)); }
GLuint GetBlurTarget5(){ return (1 + ((curBlurTarget) % 6)); }

void UpdateFrameCount()
{
	static int iFrames = 0;           // Frame count
	static CStopWatch frameTimer;     // Render time

	// Reset the stopwatch on first time
	if (iFrames == 0)
	{
		frameTimer.Reset();
		iFrames++;
	}
	// Increment the frame count
	iFrames++;

	// Do periodic frame rate calculation
	if (iFrames == 101)
	{
		float fps;

		fps = 100.0f / frameTimer.GetElapsedSeconds();
		if (bUsePBOPath)
			printf("Pix_buffs - Using PBOs  %.1f fps\n", fps);
		else
			printf("Pix_buffs - Using Client mem copies %.1f fps\n", fps);

		frameTimer.Reset();
		iFrames = 1;
	}
}

bool LoadBMPTexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode){
	GLbyte *pBits;
	GLint iWidth, iHeight;

	pBits = gltReadBMPBits(szFileName, &iWidth, &iHeight);
	if (NULL == pBits)
		return false;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);

	if (minFilter == GL_LINEAR_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_NEAREST_MIPMAP_NEAREST)
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iWidth, iHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pBits);

	//free(pBits);
	return true;
}

void SetupRC(void){
	GLenum err;
	err = glewInit();
	if (GLEW_OK != err){
		fprintf(stderr, "The error of GLEW is %s", glewGetErrorString(err));
	}

	shaderManager.InitializeStockShaders();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	gltMakeTorus(torusBatch, 0.4f, 0.15f, 35, 35);
	GLfloat alpha = 0.25f;
	floorBatch.Begin(GL_TRIANGLE_FAN, 4, 1);
	floorBatch.Color4f(0.0f, 1.0f, 0.0f, alpha);
	floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
	floorBatch.Normal3f(0.0, 1.0f, 0.0f);
	floorBatch.Vertex3f(-20.0f, -0.41f, 20.0f);

	floorBatch.Color4f(0.0f, 1.0f, 0.0f, alpha);
	floorBatch.MultiTexCoord2f(0, 10.0f, 0.0f);
	floorBatch.Normal3f(0.0, 1.0f, 0.0f);
	floorBatch.Vertex3f(20.0f, -0.41f, 20.0f);

	floorBatch.Color4f(0.0f, 1.0f, 0.0f, alpha);
	floorBatch.MultiTexCoord2f(0, 10.0f, 10.0f);
	floorBatch.Normal3f(0.0, 1.0f, 0.0f);
	floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);

	floorBatch.Color4f(0.0f, 1.0f, 0.0f, alpha);
	floorBatch.MultiTexCoord2f(0, 0.0f, 10.0f);
	floorBatch.Normal3f(0.0, 1.0f, 0.0f);
	floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);
	floorBatch.End();

	glGenTextures(1, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	LoadBMPTexture("marble.bmp", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);

	blurProg = gltLoadShaderPairWithAttributes("blur.vs", "blur.fs", 2,
		GLT_ATTRIBUTE_VERTEX, "vVertex", GLT_ATTRIBUTE_TEXTURE0, "texCoord0");

	glGenTextures(6, blurTextures);

	pixelDataSize = screenHeight * screenWidth * 3 * sizeof(unsigned int);
	void* data = (void *)malloc(pixelDataSize);
	memset(data, 0x00, pixelDataSize);

	for (int i = 0; i < 6; i++){
		glActiveTexture(GL_TEXTURE1 + i);
		glBindTexture(GL_TEXTURE_2D, blurTextures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}

	glGenBuffers(1, pixBuffObjs);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pixBuffObjs[0]);
	glBufferData(GL_PIXEL_PACK_BUFFER, pixelDataSize, pixelData, GL_DYNAMIC_COPY);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	gltGenerateOrtho2DMat(screenWidth, screenHeight, orthoMatrix, screenQuad);

	glGenFramebuffers(1, &fboName);
	
	glGenRenderbuffers(1, &depthBufferName);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufferName);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, screenWidth, screenHeight);

	glGenRenderbuffers(3, renderBufferNames);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferNames[0]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferNames[1]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferNames[2]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, screenWidth, screenHeight);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboName);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferName);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBufferNames[0]);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, renderBufferNames[1]);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, renderBufferNames[2]);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	gltCheckErrors();
}

void ShutdownRC(void){
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	
	for (int i = 0; i < 7; i++){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glDeleteTextures(1, textures);
	glDeleteTextures(6, blurTextures);

	glDeleteRenderbuffers(1, &depthBufferName);
	glDeleteRenderbuffers(3, renderBufferNames);

	glDeleteFramebuffers(1, &fboName);

	glDeleteBuffers(1, pixBuffObjs);
}

void ChangeSize(int nWidth, int nHeight){
	glViewport(0, 0, nWidth, nHeight);
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);

	viewFrustum.SetPerspective(35.0f, float(nWidth) / float(nHeight), 1.0f, 100.0f);
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
	modelViewMatrix.LoadIdentity();

	screenHeight = nHeight;
	screenWidth = nWidth;

	gltGenerateOrtho2DMat(screenWidth, screenHeight, orthoMatrix, screenQuad);

	free(pixelData);
	pixelDataSize = screenHeight * screenWidth * 3 * sizeof(unsigned int);
	pixelData = (void*)malloc(pixelDataSize);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pixBuffObjs[0]);
	glBufferData(GL_PIXEL_PACK_BUFFER, pixelDataSize, pixelData, GL_DYNAMIC_COPY);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, depthBufferName);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferNames[0]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferNames[1]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferNames[2]);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, screenWidth, screenHeight);

	gltCheckErrors();
}

void ProccessKeys(unsigned char key, int x, int y)
{
	static CStopWatch cameraTimer;
	float fTime = cameraTimer.GetElapsedSeconds();
	float linear = fTime * 12.0f;
	cameraTimer.Reset();

	// Alternate between PBOs and local memory when 'P' is pressed
	if (key == 'P' || key == 'p')
		bUsePBOPath = (bUsePBOPath) ? GL_FALSE : GL_TRUE;

	// Speed up movement
	if (key == '+')
	{
		speedFactor += linear / 2;
		if (speedFactor > 6)
			speedFactor = 6;
	}

	// Slow down moement
	if (key == '-')
	{
		speedFactor -= linear / 2;
		if (speedFactor < 0.5)
			speedFactor = 0.5;
	}

	if (key == GLUT_KEY_F3)
	{
		//curBuffs = GL_COLOR_ATTACHMENT0;
	}
	else if (key == GLUT_KEY_F4)
	{
		curBuffs = GL_COLOR_ATTACHMENT1;
	}
	else if (key == GLUT_KEY_F5)
	{
		curBuffs = GL_COLOR_ATTACHMENT2;
	}
}

void SetupBlurProg(void)
{
	// Set the blur program as the current one
	glUseProgram(blurProg);

	// Set MVP matrix
	glUniformMatrix4fv(glGetUniformLocation(blurProg, "mvpMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// Setup the textue units for the blur targets, these rotate every frame
	glUniform1i(glGetUniformLocation(blurProg, "textureUnit0"), GetBlurTarget0());
	glUniform1i(glGetUniformLocation(blurProg, "textureUnit1"), GetBlurTarget1());
	glUniform1i(glGetUniformLocation(blurProg, "textureUnit2"), GetBlurTarget2());
	glUniform1i(glGetUniformLocation(blurProg, "textureUnit3"), GetBlurTarget3());
	glUniform1i(glGetUniformLocation(blurProg, "textureUnit4"), GetBlurTarget4());
	glUniform1i(glGetUniformLocation(blurProg, "textureUnit5"), GetBlurTarget5());
}

void DrawWorld(GLfloat yRot, GLfloat xPos){
	M3DMatrix44f mCamera;
	modelViewMatrix.GetMatrix(mCamera);

	// Need light position relative to the Camera
	M3DVector4f vLightTransformed;
	m3dTransformVector4(vLightTransformed, vLightPos, mCamera);

	modelViewMatrix.PushMatrix();
		modelViewMatrix.Translate(0.0f, 0.2f, -2.5f);
		modelViewMatrix.Translate(xPos, 0.0f, 0.0f);
		modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
		shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,
			modelViewMatrix.GetMatrix(),
			transformPipeline.GetProjectionMatrix(),
			vLightTransformed, vGreen, 0);
		torusBatch.Draw();
	modelViewMatrix.PopMatrix();
}

void RenderScene(void){
	static CStopWatch animationTimer;
	static float totalTime = 6; // To go back and forth
	static float halfTotalTime = totalTime / 2;
	float seconds = animationTimer.GetElapsedSeconds() * speedFactor;
	float xPos = 0;

	// Calculate the next postion of the moving object
	// First perform a mod-like operation on the time as a float
	while (seconds > totalTime)
		seconds -= totalTime;

	// Move object position, if it's gone half way across
	// start bringing it back
	if (seconds < halfTotalTime)
		xPos = seconds - halfTotalTime*0.5f;
	else
		xPos = totalTime - seconds - halfTotalTime*0.5f;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboName);
	glDrawBuffers(3, fboBuffs);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	modelViewMatrix.PushMatrix();
		M3DMatrix44f mCamera;
		cameraFrame.GetCameraMatrix(mCamera);
		modelViewMatrix.MultMatrix(mCamera);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE, transformPipeline.GetModelViewProjectionMatrix(), vWhite, 0);

		floorBatch.Draw();
		DrawWorld(0.0, xPos);
	modelViewMatrix.PopMatrix();

	if (bUsePBOPath){
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pixBuffObjs[0]);
		glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixBuffObjs[0]);
		glActiveTexture(GL_TEXTURE0 + GetBlurTarget0());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}
	else
	{
		// Grab the screen pixels and copy into local memory
		glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);

		// Push pixels from client memory into texture
		// Setup texture unit for new blur, this gets imcremented every frame
		glActiveTexture(GL_TEXTURE0 + GetBlurTarget0());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
	}

	projectionMatrix.PushMatrix();
		projectionMatrix.LoadIdentity();
		projectionMatrix.MultMatrix(orthoMatrix);
		modelViewMatrix.PushMatrix();
			modelViewMatrix.LoadIdentity();
			glDisable(GL_DEPTH_TEST);
			SetupBlurProg();
			screenQuad.Draw();
			glEnable(GL_DEPTH_TEST);
		modelViewMatrix.PopMatrix();
	projectionMatrix.PopMatrix();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDrawBuffers(1, windowBuff);
	glViewport(0, 0, screenWidth, screenHeight);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboName);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, screenWidth, screenHeight,
		0, 0, screenWidth, screenHeight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	//glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glReadBuffer(curBuffs);
	glBlitFramebuffer(0, 0, screenWidth, screenHeight,
		(int)(screenWidth *(0.8)), (int)(screenHeight*(0.8)),
		screenWidth, screenHeight,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);

	AdvanceBlurTaget();
	
	glutSwapBuffers();

	glutPostRedisplay();

	UpdateFrameCount();
}


int main(int argc, char* argv[])
{
	screenWidth = 800;
	screenHeight = 600;
	bFullScreen = false;
	bAnimated = true;
	bUsePBOPath = false;
	blurProg = 0;
	speedFactor = 1.0f;
	fboName = 0;
	depthBufferName = 0;

	gltSetWorkingDirectory(argv[0]);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(screenWidth, screenHeight);

	glutCreateWindow("Pix Buffs");

	glutReshapeFunc(ChangeSize);
	glutDisplayFunc(RenderScene);
	glutKeyboardFunc(ProccessKeys);

	SetupRC();
	glutMainLoop();
	ShutdownRC();
	return 0;
}
