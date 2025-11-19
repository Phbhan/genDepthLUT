//
// Created by tinntt on 21/06/2021.
//


#include "shader.h"

#include <stdio.h>
//#include <log/log.h>

#include <memory>

// Given shader source, load and compile it
static GLuint loadShader(GLenum type, const char *shaderSrc, const char *name) {
    // Create the shader object
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    // Load and compile the shader
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    // Verify the compilation worked as expected
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printf("Error compiling %s shader for %s\n", (type == GL_VERTEX_SHADER) ? "vtx" : "pxl",
               name);

        GLint size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            // Get and report the error message
            std::unique_ptr<char> infoLog(new char[size]);
            glGetShaderInfoLog(shader, size, NULL, infoLog.get());
            printf("  msg:\n%s\n", infoLog.get());
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint loadShaderFromFile(GLenum type, const char *shader_path, const char *name) {
    bool m_bOpen;
    GLint       size;
    const char* data = NULL;

    FILE *pFile = fopen(shader_path, "rb");
    if (pFile) {
        // Get the file size
        fseek(pFile, 0, SEEK_END);
        size = ftell(pFile);
        if (size < 0) {
            //ALOGE(" size is negative");
            fclose(pFile);
            return 0;
        }
        fseek(pFile, 0, SEEK_SET);

        // read the data, append a 0 byte as the data might represent a string
        char *pData = new char[size + 1];
        pData[size] = '\0';
        int BytesRead = fread(pData, 1, size, pFile);

        if (BytesRead != size) {
            delete[] pData;
            size = 0;
        } else {
            data = pData;
            m_bOpen = true;
        }
        fclose(pFile);

    } else {
        size = 0;
    }

    // Create the shader object
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        delete[] data;
        return 0;
    }

    // Load and compile the shader
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    // Verify the compilation worked as expected
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printf("Error compiling %s shader for %s\n", (type == GL_VERTEX_SHADER) ? "vtx" : "pxl",
               name);

        GLint size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            // Get and report the error message
            std::unique_ptr<char> infoLog(new char[size]);
            glGetShaderInfoLog(shader, size, NULL, infoLog.get());
            printf("  msg:\n%s\n", infoLog.get());
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint buildShaderProgram(const char *vtxSrc, const char *pxlSrc, const char *name) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        //ALOGE("Failed to allocate program object\n");
        return 0;
    }

    // Compile the shaders and bind them to this program
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vtxSrc, name);
    if (vertexShader == 0) {
        //ALOGE("Failed to load vertex shader\n");
        glDeleteProgram(program);
        return 0;
    }
    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pxlSrc, name);
    if (pixelShader == 0) {
        //ALOGE("Failed to load pixel shader\n");
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        return 0;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);

    // Link the program
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        //ALOGE("Error linking program.\n");
        GLint size = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            // Get and report the error message
            std::unique_ptr<char> infoLog(new char[size]);
            glGetProgramInfoLog(program, size, NULL, infoLog.get());
            printf("  msg:  %s\n", infoLog.get());
        }

        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(pixelShader);
        return 0;
    }


#if 0 // Debug output to diagnose shader parameters
    GLint numShaderParams;
    GLchar paramName[128];
    GLint paramSize;
    GLenum paramType;
    const char *typeName = "?";
    printf("Shader parameters for %s:\n", name);
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numShaderParams);
    for (GLint i=0; i<numShaderParams; i++) {
        glGetActiveUniform(program,
                           i,
                           sizeof(paramName),
                           nullptr,
                           &paramSize,
                           &paramType,
                           paramName);
        switch (paramType) {
            case GL_FLOAT:      typeName = "GL_FLOAT"; break;
            case GL_FLOAT_VEC4: typeName = "GL_FLOAT_VEC4"; break;
            case GL_FLOAT_MAT4: typeName = "GL_FLOAT_MAT4"; break;
            case GL_SAMPLER_2D: typeName = "GL_SAMPLER_2D"; break;
        }

        printf("  %2d: %s\t (%d) of type %s(%d)\n", i, paramName, paramSize, typeName, paramType);
    }
#endif

    return program;
}


// Create a program object given vertex and pixels shader source
GLuint buildShaderProgramFromFile(const char *vtxSrcPath, const char *pxlSrcPath, const char *name) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        //ALOGE("Failed to allocate program object\n");
        return 0;
    }

    // Compile the shaders and bind them to this program
    GLuint vertexShader = loadShaderFromFile(GL_VERTEX_SHADER, vtxSrcPath, name);
    if (vertexShader == 0) {
        //ALOGE("Failed to load vertex shader\n");
        glDeleteProgram(program);
        return 0;
    }
    GLuint pixelShader = loadShaderFromFile(GL_FRAGMENT_SHADER, pxlSrcPath, name);
    if (pixelShader == 0) {
        //ALOGE("Failed to load pixel shader\n");
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        return 0;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);

    // Link the program
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        //ALOGE("Error linking program.\n");
        GLint size = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            // Get and report the error message
            std::unique_ptr<char> infoLog(new char[size]);
            glGetProgramInfoLog(program, size, NULL, infoLog.get());
            printf("  msg:  %s\n", infoLog.get());
        }

        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(pixelShader);
        return 0;
    }


#if 0 // Debug output to diagnose shader parameters
    GLint numShaderParams;
    GLchar paramName[128];
    GLint paramSize;
    GLenum paramType;
    const char *typeName = "?";
    printf("Shader parameters for %s:\n", name);
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numShaderParams);
    for (GLint i=0; i<numShaderParams; i++) {
        glGetActiveUniform(program,
                           i,
                           sizeof(paramName),
                           nullptr,
                           &paramSize,
                           &paramType,
                           paramName);
        switch (paramType) {
            case GL_FLOAT:      typeName = "GL_FLOAT"; break;
            case GL_FLOAT_VEC4: typeName = "GL_FLOAT_VEC4"; break;
            case GL_FLOAT_MAT4: typeName = "GL_FLOAT_MAT4"; break;
            case GL_SAMPLER_2D: typeName = "GL_SAMPLER_2D"; break;
        }

        printf("  %2d: %s\t (%d) of type %s(%d)\n", i, paramName, paramSize, typeName, paramType);
    }
#endif

    return program;
}
