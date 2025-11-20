/* Copyright (C) 2021 VINAI Artificial Intelligence Application and Research JSC - All Rights Reserved*/

#include "main.h"

#define STITCH_IMAGE_WIDTH 1280
#define STITCH_IMAGE_HEIGHT 800
#define STITCH_IMAGE_COLORSPACE 3

using namespace std;
int main(int argc, char *argv[])
{
	std::cout << "Gen Top Lut\n";

	init();
    createProgram();

	std::string calibPath = "../data/cameraData.json";
	loadCalib(calibPath.c_str());

	genMesh(meshWidth, meshHeight, 200, 200);
	genUV();
	createVBOs();

	draw(0);
	draw(1);
	draw(2);
	draw(3);

	deinit();
	return 0;
}

void init(){
    // Initialize EGL
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisplay, nullptr, nullptr);

    // Set up EGL attributes for the context
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig eglConfig;
    EGLint numConfigs;
    eglChooseConfig(eglDisplay, attribs, &eglConfig, 1, &numConfigs);

    // Create a Pbuffer surface for offscreen rendering
    EGLint surfaceAttribs[] = { EGL_WIDTH, IMG_WIDTH, EGL_HEIGHT, IMG_HEIGHT, EGL_NONE };
    eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, surfaceAttribs);

    // Create an EGL context
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);

    // Make the context current
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

    // Create a framebuffer and a texture
    
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &texture);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IMG_WIDTH, IMG_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Check for framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
        return;
    }
}

void deinit(){
    glDeleteProgram(programRenderPanorama);
	glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &framebuffer);
	glDeleteBuffers(N_CAM, &vertexBuffers);
	glDeleteBuffers(N_CAM, &uvBuffers[0]);
}

bool createProgram(){
    programRenderPanorama =
            buildShaderProgram(BOWL_VERT_SHADER, BOWL_FRAG_SHADER,
                                       "programRenderPanorama");
	if (!programRenderPanorama) {
        std::cerr << "programRenderPanorama is not complete!" << std::endl;
    } else {
		glUseProgram(programRenderPanorama);
		glUniform1f(glGetUniformLocation(programRenderPanorama, "meshW"), meshWidth);
        glUniform1f(glGetUniformLocation(programRenderPanorama, "meshH"), meshHeight);
        std::cerr << "programRenderPanorama finished!" << std::endl;
    }
    return true;
}

bool loadCalib(const char* configFileName) {
    bool complete = true;

    // Set up a stream to read in the input file
    std::ifstream configStream(configFileName);

    // Parse the stream into JSON objects
    JsonLib::Reader reader;
    JsonLib::Value rootNode;
    bool parseOk = reader.parse(configStream, rootNode, false /* don't need comments */);
    if (!parseOk) {
        LOGE("Failed to read configuration file %s\n", configFileName);
        LOGE("%s\n", reader.getFormatedErrorMessages().c_str());
        return false;
    }

    {
        JsonLib::Value cameraArray = rootNode["Items"];
        if (!cameraArray.isArray()) {
            LOGE("Invalid configuration format -- we expect an array of cameras\n");
            return false;
        }

        mCalibs.reserve(cameraArray.size());
        for (auto &&node: cameraArray) {
            // Get data from the configuration file
            float matrixD[4];
            int index = 0;
            JsonLib::Value matrixDArr = node.get("matrixD", "MISSING");
            for (auto &&node: matrixDArr) {
                matrixD[index] = node.asFloat();
                index ++;
            }

            float matrixK[4];
            index = 0;
            JsonLib::Value matrixKArr = node.get("matrixK", "MISSING");
            for (auto &&node: matrixKArr) {
                matrixK[index] = node.asFloat();
                index ++;
            }

            float matrixR[9];
            index = 0;
            JsonLib::Value matrixRArr = node.get("matrixR", "MISSING");
            for (auto &&node: matrixRArr) {
                matrixR[index] = node.asFloat();
                index ++;
            }

            float vectorT[3];
            index = 0;
            JsonLib::Value vectorTArr = node.get("vectT", "MISSING");
            for (auto &&node: vectorTArr) {
                vectorT[index] = node.asFloat();
                index ++;
            }
            int camPosData   = node.get("camPos", 0).asInt();

            CameraIF info;
            std::memcpy(info.matrixD, matrixD, sizeof(matrixD));
            std::memcpy(info.matrixK, matrixK, sizeof(matrixK));
            std::memcpy(info.matrixR, matrixR, sizeof(matrixR));
            std::memcpy(info.vectT, vectorT, sizeof(vectorT));
            info.camPos = camPosData;
            mCalibs.push_back(info);
        }
    }
	std::cout << "loadCalib Finished\n";
    return complete;
}

bool genMesh(float width, float height, int M, int N){
    float dx = height / N;
    float dy = width / M;
    mVerticeSize = M  * N * 6;
    mVertices = Eigen::MatrixXf( 3, mVerticeSize);
    int index = 0;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float x = j * dx - height / 2.0f;
            float y = i * dy - width / 2.0f;

            mVertices(0, index) = x;
            mVertices(1, index) = y;
            mVertices(2, index) = 0.0f;
			index ++;

            mVertices(0, index) = x;
            mVertices(1, index) = y + dy;
            mVertices(2, index) = 0.0f;
			index ++;

            mVertices(0, index) = x + dx;
            mVertices(1, index) = y + dy;
            mVertices(2, index) = 0.0f;
			index ++;

            mVertices(0, index) = x;
            mVertices(1, index) = y;
            mVertices(2, index) = 0.0f;
			index ++;

            mVertices(0, index) = x + dx;
            mVertices(1, index) = y;
            mVertices(2, index) = 0.0f;
			index ++;

            mVertices(0, index) = x + dx;
            mVertices(1, index) = y + dy;
            mVertices(2, index) = 0.0f;
			index ++;
		}
    }
	std::cout << "genMesh Finished\n";
    return true;
}

bool genUV(){
	for (int i = 0; i < N_CAM; ++i) {
        Eigen::MatrixXf uvOuts;
        if (genUVFromBowl(mVertices, mCalibs[i], uvOuts) == false) {
            LOGD("Cannot found calib information");
            break;
        }
        mListOfUVs.push_back(uvOuts);
    }
	std::cout << "genUV Finished\n";
	return true;
}

bool genUVFromBowl(Eigen::MatrixXf vecWorld, CameraIF camIF, Eigen::MatrixXf &out_uvs){
    int verticeSize = vecWorld.cols();
    Eigen::MatrixXf MatrixR;
    MatrixR   = Conv_Vector9x1fEigen3x3f(camIF.matrixR);

    // vecTransformed
    //rotate
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vecTransformed;
    vecTransformed = MatrixR * vecWorld;

    //translation
    vecTransformed.row(0) += camIF.vectT[0];
    vecTransformed.row(1) += camIF.vectT[1];
    vecTransformed.row(2) += camIF.vectT[2];

    //lenVec2D, lenVec3D
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> lenVec2D, lenVec3D;
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vecTransformed_p2;
    vecTransformed_p2  = vecTransformed.square();
    lenVec2D           = vecTransformed_p2.topRows(2).colwise().sum().sqrt();
    lenVec3D           = vecTransformed_p2.topRows(3).colwise().sum().sqrt();

    // ratio, theta
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> ratio;
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> theta_tmp, theta, theta_p2, theta_p4, theta_p6, theta_p8;
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> cdist;
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> xd, yd;
    ratio       = lenVec2D * lenVec3D.inverse();
    theta       = ratio.asin();
    theta_tmp   = M_PI - theta;
    Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> maskTheta;
    maskTheta   = (vecTransformed.row(2) > 0.0);
    theta       = maskTheta.select(theta, theta_tmp);

    theta_p2    = theta.square();
    theta_p4    = theta_p2.square();
    theta_p6    = theta_p2.cube();
    theta_p8    = theta_p4.square();
    cdist       = theta * (1.0 +
                    camIF.matrixD[0] * theta_p2 +
                    camIF.matrixD[1] * theta_p4 +
                    camIF.matrixD[2] * theta_p6 +
                    camIF.matrixD[3] * theta_p8);
    // xd, yd
    xd = (vecTransformed.row(0) * lenVec2D.inverse()) * cdist;
    yd = (vecTransformed.row(1) * lenVec2D.inverse()) * cdist;

    // vecPoint2D
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vecPoint2D;
    vecPoint2D  = Eigen::MatrixXf::Constant(2, verticeSize, 0.0);
    vecPoint2D.row(0) = camIF.matrixK[0] * xd + camIF.matrixK[1];
    vecPoint2D.row(1) = camIF.matrixK[2] * yd + camIF.matrixK[3];

    vecPoint2D.row(0) /= RAW_IMAGE_WIDTH;
    vecPoint2D.row(1) /= RAW_IMAGE_HEIGHT;

    // ***************
    // masking : if ((vecPoint2D[1] >= 0.0 && vecPoint2D[1] <= RAW_IMG_HEIGHT) && (vecPoint2D[0] >= 0.0 && vecPoint2D[0] <= RAW_IMG_WIDTH))
    Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> maskVecPoint2D;
    maskVecPoint2D = ((theta <= 90 * M_PI/180.0) && ((vecPoint2D.row(1) >= 0.0) && (vecPoint2D.row(1) <= 1.0)) && ((vecPoint2D.row(0) >= 0.0) && (vecPoint2D.row(0) <= 1.0)));

    // UV
    out_uvs = Eigen::MatrixXf::Constant(2, mVerticeSize, 0.0);
    out_uvs.row(0) = maskVecPoint2D.select(vecPoint2D.row(0), -2000.0);
    out_uvs.row(1) = maskVecPoint2D.select(vecPoint2D.row(1), -2000.0);

	for (int i = 0; i < verticeSize; i += 3) {
    bool hasInvalid = false;
    for (int j = 0; j < 3; ++j) {
        Eigen::Vector2f pt = out_uvs.col(i + j);
        if (pt.x() == -2000.0f || pt.y() == -2000.0f) {
            hasInvalid = true;
            break;
        }
    }
    if (hasInvalid) {
        for (int j = 0; j < 3; ++j) {
            out_uvs.col(i + j) = Eigen::Vector2f(-2000.0f, -2000.0f);
        }
    }
}

    return true;
}

Eigen::MatrixXf Conv_Vector9x1fEigen3x3f(float vector9f[9]) {
    Eigen::MatrixXf eigen3x3f      = Eigen::MatrixXf::Constant(3, 3, 0.0f);
        eigen3x3f.row(0)    = Eigen::VectorXf::Map(&vector9f[0], 3);
        eigen3x3f.row(1)    = Eigen::VectorXf::Map(&vector9f[3], 3);
        eigen3x3f.row(2)    = Eigen::VectorXf::Map(&vector9f[6], 3);

    return eigen3x3f;
}

bool createVBOs(){
    glGenBuffers(1, &vertexBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers);
    glBufferData(GL_ARRAY_BUFFER, mVerticeSize * 3 * sizeof(float),
                    mVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(N_CAM, &uvBuffers[0]);
    for (int i = 0; i < N_CAM; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[i]);
        glBufferData(GL_ARRAY_BUFFER, mVerticeSize * 2 * sizeof(float),
                    mListOfUVs[i].data(), GL_STATIC_DRAW);
    }
	std::cout << "createVBOs Finished\n";
    return true;
}

void checkLUT(std::string filename){
    cv::Mat meshgridLUT = cv::imread(filename, cv::IMREAD_COLOR); 
    cv::Mat img = cv::imread("data/img0.png", cv::IMREAD_COLOR); 
    cv::Mat roi = cv::imread("data/roi_mask_cam/roi_mask_0.png", cv::IMREAD_COLOR); 
    cv::resize(img, img, cv::Size(512, 320));
    cv::resize(roi, roi, cv::Size(512, 320));

    // Blend (apply LUT on top of image2) - alpha controls transparency
    double alpha = 0.25; // 0 = only img, 1 = only meshgrid
    cv::Mat blended;
    cv::addWeighted(img, 1.0 - alpha, meshgridLUT, alpha, 0.0, blended);
    cv::addWeighted(blended, 1.0 - alpha, roi, alpha, 0.0, blended);

    // Save result
    cv::imwrite("output.png", blended);

    int pixels = 320*512;
    int meshHeight = 180;
    int meshWidth = 160;

    cv::Mat projected(meshHeight, meshWidth, CV_8UC3, cv::Scalar(0,0,0));

    std::vector<float> lutX(pixels);
    std::vector<float> lutY(pixels);
    std::vector<float> lutOcc8(pixels);

    for (int v = 0; v < 320; ++v) {
        const cv::Vec3b* rowLUT = meshgridLUT.ptr<cv::Vec3b>(v);
        const cv::Vec3b* rowImg = img.ptr<cv::Vec3b>(v);

        for (int u = 0; u < 512; ++u) {
            cv::Vec3b lutPix = rowLUT[u]; // BGR (OpenCV)
            cv::Vec3b srcPix = rowImg[u];

            float B = lutPix[0]; // ground flag as in your data
            float G = lutPix[1];
            float R = lutPix[2];

            int meshX = (int)((1 - (R / 255.0f)) * (meshHeight - 1));
            int meshY = (int)((1 - (G / 255.0f)) * (meshWidth - 1));
            
            if(meshX<0 || meshY<0)
                continue;
                // std::cout<<meshX<<" " <<meshY << " " << R << " " << G << std::endl;
            if (B > 0) {
                projected.at<cv::Vec3b>(meshX, meshY) = srcPix;
            }
        }
    }
    std::cout << "saving debug img" << std::endl;
    cv::imwrite("projected.png", projected);
}

void draw(int camPos){   
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[camPos]);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, mVerticeSize);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    uint8_t* imageBuffer = new uint8_t[IMG_WIDTH * IMG_HEIGHT * 4];
    glReadPixels(0, 0, IMG_WIDTH, IMG_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
    std::string filename = "LUT_cam" + std::to_string(camPos) + ".png";
	std::cout << "save file: " << filename << std::endl;;
    int output = save_png(filename, IMG_WIDTH, IMG_HEIGHT, 8, PNG_COLOR_TYPE_RGBA, &imageBuffer[0], 4*IMG_WIDTH, PNG_TRANSFORM_IDENTITY);
    if (camPos==0)
    {   
        checkLUT(filename);
    }
}

int save_png(std::string filename, int width, int height,
                     int bitdepth, int colortype,
                     uint8_t* data, int pitch, int transform){
    /*
    save_png
    -----------------------------------------------------
    A simple to save a png with a bit more flexibility. This function
    returns 0 on success otherwise -1.
    - filename:   the path where you want to save the png.
    - width:      width of the image
    - height:     height of the image
    - bitdepth:   how many bits per pixel (e.g. 8).
    - colortype:  PNG_COLOR_TYEP_GRAY
                  PNG_COLOR_TYPE_PALETTE
                  PNG_COLOR_TYPE_RGB
                  PNG_COLOR_TYPE_RGB_ALPHA
                  PNG_COLOR_TYPE_GRAY_ALPHA
                  PNG_COLOR_TYPE_RGBA          (alias for _RGB_ALPHA)
                  PNG_COLOR_TYPE_GA            (alias for _GRAY_ALPHA)
    - pitch:      The stride (e.g. '4 * width' for RGBA).
    - transform:  PNG_TRANSFORM_IDENTITY
                  PNG_TRANSFORM_PACKING
                  PNG_TRANSFORM_PACKSWAP
                  PNG_TRANSFORM_INVERT_MONO
                  PNG_TRANSFORM_SHIFT
                  PNG_TRANSFORM_BGR
                  PNG_TRANSFORM_SWAP_ALPHA
                  PNG_TRANSFORM_SWAP_ENDIAN
                  PNG_TRANSFORM_INVERT_ALPHA
                  PNG_TRANSFORM_STRIP_FILLER
    */
    int i = 0;
    int r = 0;
    FILE* fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep* row_pointers = NULL;
    if (NULL == data) {
        r = 1;
        goto error;
    }
    if (0 == pitch) {
        r = 3;
        goto error;
    }
    fp = fopen(filename.c_str(), "wb");
    if (NULL == fp) {
        r = 4;
        goto error;
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (NULL == png_ptr) {
        r = 5;
        goto error;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (NULL == info_ptr) {
        r = 6;
        goto error;
    }
    png_set_IHDR(png_ptr,
                info_ptr,
                width,
                height,
                bitdepth,                 /* e.g. 8 */
                colortype,                /* PNG_COLOR_TYPE_{GRAY, PALETTE, RGB, RGB_ALPHA, GRAY_ALPHA, RGBA, GA} */
                PNG_INTERLACE_NONE,       /* PNG_INTERLACE_{NONE, ADAM7 } */
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);
    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (i = 0; i < height; ++i) {
        row_pointers[i] = data + i * pitch;
    }
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, transform, NULL);
    error:
    if (NULL != fp) {
        fclose(fp);
        fp = NULL;
    }
    if (NULL != png_ptr) {
        if (NULL == info_ptr) {
            r = 7;
            return r;
        }
        png_destroy_write_struct(&png_ptr, &info_ptr);
        png_ptr = NULL;
        info_ptr = NULL;
    }
    if (NULL != row_pointers) {
        free(row_pointers);
        row_pointers = NULL;
    }
    return r;
}

