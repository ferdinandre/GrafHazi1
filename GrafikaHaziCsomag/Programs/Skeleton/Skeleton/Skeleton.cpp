//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Ferdinánd André Albert
// Neptun : G6MHH3
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders

//----//Necessary funcitons
vec3 hyperbolicCross(vec3 p, vec3 v) {
	float a = (p.y * -1.0f * v.z) - (-1.0f * p.z * v.y);
	float b = (v.x * -1.0f * p.z) - (p.x * -1.0f * v.z);
	float c = p.x * v.y - v.x * p.y;
	return vec3(a, b, c);
}
float Lorentz(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y - a.z * b.z;
}
vec3 normalizeVector(vec3 v) {
	float k = sqrtf(Lorentz(v, v));
	return v / k;
}
vec3 movePoint(vec3 p, vec3 spv, float d) {
	return p * coshf(d) + spv * sinhf(d);
}
vec3 updateSPV(vec3 spv, vec3 mp, float d) {
	return mp * sinhf(d) + spv * coshf(d);
}
vec3 vectorReproject(vec3 v, vec3 p) {
	return v + Lorentz(v, p) * p;
}
vec3 perp(vec3 p, vec3 v) {
	return vectorReproject(normalizeVector(hyperbolicCross(p, v)), p);
}
vec3 rotateByTheta(vec3 v, vec3 p, float theta) {
	vec3 v_ = perp(v, p);
	return v * cosf(theta) + v_ * sinf(theta);
}
vec3 centralPointReproject(vec3 p) {
	float lambda = sqrtf(-1.0f / Lorentz(p, p));
	return lambda * p;
}
vec3 perpendicularPointReproject(vec2 p) {
	float w = sqrtf(p.x * p.x + p.y * p.y + 1.0f);
	return vec3(p.x, p.y, w);
}
vec2 poincareProjection(vec3 v) {
	float a = v.x / (v.z + 1);
	float b = v.y / (v.z + 1);
	return vec2(a, b);
}
vec3 getSpeedVector(float x, float y, vec3 p) {
	float z = (x * p.x + y * p.y) / p.z;
	return vec3(x, y, z);
}
float hyp_dist(vec3 p, vec3 q) {
	
	return acoshf(Lorentz(-1.0f * p, q));

}
vec3 speedVectorTwoPoints(vec3 p, vec3 q) {
	float d = hyp_dist(p, q);
	vec3 result = (q - p * coshf(d)) / sinhf(d);
	
	return result;
}
//----//End of Necessary Functions
static const int nv = 100; //Number of vertices in each circle
static const int starvertices = 20;
static int returnSameNumber(int n) {
	return n;
}


class Circle {
public:
	unsigned int vao = 0 ,vbo = 0;
	Circle() {}
	void init() {
		glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
	}
	void draw() {
		glBindVertexArray(vao);
		vec2 vertexArray[nv];
		for (int i = 0; i < nv; ++i) {
			float fi = i * 2 * M_PI / nv;
			vertexArray[i] = vec2(cosf(fi), sinf(fi));
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * nv, vertexArray, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 0.0f, 0.0f, 0.0f);
		float MVPtransf[4][4] = { 1, 0, 0, 0,
								  0, 1, 0, 0,
								  0, 0, 1, 0,
								  0, 0, 0, 1 };
		location = glGetUniformLocation(gpuProgram.getId(), "MVP");
		glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);
		glDrawArrays(GL_TRIANGLE_FAN, 0, nv);
	}
};
class Hami {
public:
	unsigned int vao = 0;
	unsigned int vboBody = 0;
	unsigned int vboMouth = 0;
	unsigned int vboEye1 = 0;
	unsigned int vboEye2 = 0;
	unsigned int vboNyal = 0;
	unsigned int vboPupil1 = 0;
	unsigned int vboPupil2 = 0;
	float velocity;
	float radius;
	float mouthRadius;
	float rotation;
	vec3 mp, spv, c, referencePoint;
	//Midpoint, Speedvector, color, midpoint of the other "hami"
	vec2 trail[2000]; int count = 0; //Vertices of the drawn trail, number of vertices
	void calculateCircle(vec3 midp, float r, vec2* arr) {
		for (int i = 0; i < nv; ++i) {
			float fi = i * 2 * M_PI / nv;
			vec3 speedvector = normalizeVector(vectorReproject(getSpeedVector(1.0f, 0.0f, midp), midp));
			speedvector = rotateByTheta(speedvector, midp, fi);
			speedvector = vectorReproject(speedvector, midp);
			arr[i] = poincareProjection(centralPointReproject(movePoint(midp, speedvector, r)));
		}
	}
	Hami(vec2 tdmp, vec3 color, float rot) :c(color) {
		mp = perpendicularPointReproject(tdmp);
		spv = vectorReproject(normalizeVector(getSpeedVector(0.0f, -1.0f, mp)), mp);
		radius = 0.3f; velocity = 0.0f;
		rotation = rot;
		referencePoint = vec3(0.0f, 0.0f, 0.0f);
		resizeMouth(0);
	}
	void init() {
		glGenVertexArrays(1, &vao);	
		glGenBuffers(1, &vboBody);
		glGenBuffers(1, &vboMouth);
		glGenBuffers(1, &vboEye1);
		glGenBuffers(1, &vboEye2);
		glGenBuffers(1, &vboPupil1);
		glGenBuffers(1, &vboPupil2);
		glGenBuffers(1, &vboNyal);
	}
	void addNyal(vec2 p) {
		if (velocity != 0.0f) {
			if (count < 2000) {
				trail[count++] = p;
			}
			else {
				for (int i = 0; i < 1999; ++i) {
					trail[i] = trail[i + 1];
				}
				trail[1999] = p;
			}
		}
	}
	void move(long dt) {
		for (long i = 0; i < dt; i += 16) {  
			mp = centralPointReproject(movePoint(mp, spv, velocity));
			spv = vectorReproject(normalizeVector(updateSPV(spv, mp, velocity)), mp);
			//One movements
			spv = rotateByTheta(spv, mp, rotation);
			//And one rotation
			addNyal(poincareProjection(mp));
		}
		vec3 referencePointVector = speedVectorTwoPoints(mp, referencePoint);
	}
	
	void drawObject(vec2* vertexArray, vec3 color, int vbo) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER,sizeof(vec2) * nv,vertexArray,GL_STATIC_DRAW);	
		glEnableVertexAttribArray(0); 
		glVertexAttribPointer(0,2, GL_FLOAT, GL_FALSE,0, NULL); 
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, color.x, color.y, color.z);
		float MVPtransf[4][4] = { 1, 0, 0, 0,
								  0, 1, 0, 0,
								  0, 0, 1, 0,
								  0, 0, 0, 1 };
		location = glGetUniformLocation(gpuProgram.getId(), "MVP");	
		glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	
		glDrawArrays(GL_TRIANGLE_FAN, 0 , nv);
	}
	void resizeMouth(long time) {
		mouthRadius = 0.075f * (0.5f * sinf((float)time / 500) + 0.5f) + 0.04f;
	}
	vec3 getMouthMidpoint() {
		return centralPointReproject(movePoint(mp, spv, radius));
	}
	vec3 getEyeMP(bool b) {
		float rot_angle = b ? M_PI / 6.0f : M_PI / -6.0f;
		vec3 rotated = rotateByTheta(spv, mp, rot_angle);
		return centralPointReproject(movePoint(mp, rotated, radius* 0.8f));
	}
	vec3 getPupilMP(vec3 eyeMP) {
		vec3 v = speedVectorTwoPoints( eyeMP, referencePoint);
		v = vectorReproject(normalizeVector(v), eyeMP);
		return centralPointReproject(movePoint(eyeMP, v, 0.08f));
	}
	void drawNyal() {
		
		glBindBuffer(GL_ARRAY_BUFFER , vboNyal);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * count, trail, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		float MVPtransf[4][4] = { 1, 0, 0, 0,
								  0, 1, 0, 0,
								  0, 0, 1, 0,
								  0, 0, 0, 1 };
		location = glGetUniformLocation(gpuProgram.getId(), "MVP");
		glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);
		glDrawArrays(GL_LINE_STRIP, 0, count);
	}
	void draw() {
		glBindVertexArray(vao);  
		vec2 body[nv]; //Test csúcsai
		calculateCircle(mp, radius, body); //Test csúcsaianak kiszámolása

		vec2 mouth[nv]; //Száj csúcsai
		vec3 mouthmp = getMouthMidpoint(); //Száj középpontjának kiszámolása
		calculateCircle(mouthmp, mouthRadius, mouth); //Száj csúcsainak kiszámolása

		vec3 eye1 = getEyeMP(true);
		vec2 eye1v[nv];
		calculateCircle(eye1, radius / 4.0f, eye1v);

		vec3 eye2 = getEyeMP(false);
		vec2 eye2v[nv];
		calculateCircle(eye2, radius / 4.0f, eye2v);

		vec3 pupil1 = getPupilMP(eye1);
		vec2 pupil1v[nv];
		calculateCircle(pupil1, radius / 8.0f, pupil1v);

		vec3 pupil2 = getPupilMP(eye2);
		vec2 pupil2v[nv];
		calculateCircle(pupil2, radius / 8.0f, pupil2v);
		
		drawObject(body, c, vboBody);
		drawObject(mouth, vec3(0, 0, 0), vboMouth);
		drawObject(eye1v, vec3(1.0f, 1.0f, 1.0f), vboEye1);
		drawObject(pupil1v, vec3(0, 0, 0.7f), vboPupil1);
		drawObject(eye2v, vec3(1.0f, 1.0f, 1.0f), vboEye2);
		drawObject(pupil2v, vec3(0, 0, 0.7f), vboPupil2);
	}
};
Circle space;
Hami c = Hami(vec2(0.0f, -0.1f), vec3(0.7f, 0.1f, 0.2f), 0);
Hami b = Hami(vec2(-0.81415f, 0.41415f), vec3(0.1f, 0.6f, 0.3f),0);



void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	c.referencePoint= b.mp;
	b.referencePoint = c.mp;
	space.init();
	c.init();
	b.init();
	b.rotation = M_PI / 180.0f;
	b.velocity = 0.015f;

	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}
void onDisplay() {
	glClearColor(0.45f, 0.45f, 0.45f, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer
	space.draw();
	c.drawNyal();
	b.drawNyal();
	b.draw();
	c.draw();
	
	
	glutSwapBuffers(); // exchange buffers for double buffering
}
long previousRenderTime = 0;
void onKeyboard(unsigned char key, int pX, int pY) {
	long time = glutGet(GLUT_ELAPSED_TIME); 
	long dt = time - previousRenderTime;
	switch (key) {
		case's': c.rotation = M_PI / -160.0f; break;
		case'f':  c.rotation = M_PI / 160.0f; break;
		case 'e': c.velocity = 0.015f; break;
	}
	c.resizeMouth(time);
	b.resizeMouth(time);
}
void onKeyboardUp(unsigned char key, int pX, int pY) {
	switch (key) {
	case's':  c.rotation = 0; break;
	case'f':  c.rotation = 0; break;
	case 'e': c.velocity = 0.0f; break;

	}
}
void onMouseMotion(int pX, int pY) {	
	float cX = 2.0f * pX / windowWidth - 1;	
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}
void onMouse(int button, int state, int pX, int pY) { 
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;
	char * buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}
	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);  
	long dt = time - previousRenderTime;
	previousRenderTime = time;
	c.resizeMouth(time);
	b.resizeMouth(time);
	c.move(dt);
	if (c.velocity != 0.0f) {
		b.referencePoint = c.mp;
	}
	b.move(dt);
	c.referencePoint = b.mp;
	glutPostRedisplay();

}
