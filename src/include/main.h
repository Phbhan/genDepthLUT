#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include "json/json.h"
#include <Eigen/Dense>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>
#include "shader.h"
#include "png.h"
#include "shader_resource.h"
#include <opencv2/opencv.hpp>

using namespace std;
#define N_CAM 4
#define SAFE_DELETE(a) if( (a) != NULL ) delete (a); (a) = NULL;
#define RAW_IMAGE_WIDTH 1280
#define RAW_IMAGE_HEIGHT 800

#define LOGE(...)        \
    printf(__VA_ARGS__); \
    printf("\n")
#define LOGD(...)        \
    printf(__VA_ARGS__); \
    printf("\n")
#define LOGI(...)        \
    printf(__VA_ARGS__); \
    printf("\n")

EGLDisplay eglDisplay;
EGLSurface eglSurface;
EGLContext eglContext;
GLuint framebuffer, texture;

const unsigned int IMG_WIDTH = 512;
const unsigned int IMG_HEIGHT = 320;
const float meshWidth = 16;
const float meshHeight = 18;

struct CameraIF {
    float matrixR[9];
    float vectT[3];
    float matrixK[4];
    float matrixD[4];
    int camPos;
};

std::vector<CameraIF> mCalibs;
Eigen::MatrixXf mVertices;
std::vector<Eigen::MatrixXf> mListOfUVs;
GLint programRenderPanorama = 0;
int mVerticeSize;
GLuint vertexBuffers;
GLuint uvBuffers[N_CAM];

void init();
void deinit();
bool createProgram();
bool loadCalib(const char* configFileName);
bool genMesh(float width, float height, int M, int N);
bool genUV();
bool genUVFromBowl(Eigen::MatrixXf vecWorld, CameraIF camIF, Eigen::MatrixXf &out_uvs);
bool createVBOs();
void draw(int camPos);

Eigen::MatrixXf Conv_Vector9x1fEigen3x3f(float vector9f[9]);

int save_png(std::string filename, int width, int height,
                     int bitdepth, int colortype,
                     uint8_t* data, int pitch, int transform);