/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/scummsys.h"

#ifdef USE_GLES2

#include "backends/graphics/opengl/shader.h"
#include "backends/graphics/opengl/debug.h"

#include "common/textconsole.h"
#include "common/system.h"

namespace OpenGL {
namespace {
const char *const s_vertexShaderSource =
    "attribute vec4 position;\n"
    "attribute vec2 texCoordIn;\n"
    "attribute vec4 blendColorIn;\n"
    "\n"
    "uniform mat4 projection;\n"
    "\n"
    "varying vec2 texCoord;\n"
    "varying vec4 blendColor;\n"
    "\n"
    "void main(void) {\n"
    "\ttexCoord    = texCoordIn;\n"
    "\tblendColor  = blendColorIn;\n"
    "\tgl_Position = projection * position;\n"
    "}\n";

const char *const s_fragmentShaderSource =
    "varying lowp vec2 texCoord;\n"
    "varying lowp vec4 blendColor;\n"
    "\n"
    "uniform sampler2D texture;\n"
    "\n"
    "void main(void) {\n"
    "\tgl_FragColor = blendColor * texture2D(texture, texCoord);\n"
    "}\n";

GLuint compileShader(const char *source, GLenum shaderType) {
	GLuint handle;
	GLCALL(handle = glCreateShader(shaderType));
	if (!handle) {
		return 0;
	}

	GLCALL(glShaderSource(handle, 1, (const GLchar **)&source, nullptr));
	GLCALL(glCompileShader(handle));

	GLint result;
	GLCALL(glGetShaderiv(handle, GL_COMPILE_STATUS, &result));
	if (result == GL_FALSE) {
		GLint logSize;
		GLCALL(glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logSize));
		GLchar *log = new GLchar[logSize];
		GLCALL(glGetShaderInfoLog(handle, logSize, nullptr, log));
		warning("Could not compile shader \"%s\": \"%s\"", source, log);
		delete[] log;
		glDeleteShader(handle);
		return 0;
	} else {
		return handle;
	}
}

GLuint s_shaderProgram = 0;

GLint s_projectionLocation  = 0;

bool compileShader() {
	GLuint vertexShader = compileShader(s_vertexShaderSource, GL_VERTEX_SHADER);
	if (!vertexShader) {
		return false;
	}

	GLuint fragmentShader = compileShader(s_fragmentShaderSource, GL_FRAGMENT_SHADER);
	if (!fragmentShader) {
		GLCALL(glDeleteShader(vertexShader));
		return false;
	}

	GLCALL(s_shaderProgram = glCreateProgram());
	if (!s_shaderProgram) {
		GLCALL(glDeleteShader(vertexShader));
		GLCALL(glDeleteShader(fragmentShader));
		return false;
	}
	GLCALL(glAttachShader(s_shaderProgram, vertexShader));
	GLCALL(glAttachShader(s_shaderProgram, fragmentShader));

	GLCALL(glBindAttribLocation(s_shaderProgram, kPositionAttribLocation, "position"));
	GLCALL(glBindAttribLocation(s_shaderProgram, kTexCoordAttribLocation, "texCoordIn"));
	GLCALL(glBindAttribLocation(s_shaderProgram, kColorAttribLocation,    "blendColorIn"));

	GLCALL(glLinkProgram(s_shaderProgram));

	GLCALL(glDetachShader(s_shaderProgram, fragmentShader));
	GLCALL(glDeleteShader(fragmentShader));

	GLCALL(glDetachShader(s_shaderProgram, vertexShader));
	GLCALL(glDeleteShader(vertexShader));

	GLint result;
	GLCALL(glGetProgramiv(s_shaderProgram, GL_LINK_STATUS, &result));
	if (result == GL_FALSE) {
		GLint logSize;
		GLCALL(glGetProgramiv(s_shaderProgram, GL_INFO_LOG_LENGTH, &logSize));
		GLchar *log = new GLchar[logSize];
		GLCALL(glGetProgramInfoLog(s_shaderProgram, logSize, nullptr, log));
		warning("Could not link shader: \"%s\"", log);
		delete[] log;
		return false;
	}

	GLCALL(s_projectionLocation  = glGetUniformLocation(s_shaderProgram, "projection"));

	return (s_projectionLocation != -1);
}

void destroyShader() {
	GLCALL(glUseProgram(0));
	GLCALL(glDeleteProgram(s_shaderProgram));
	s_shaderProgram = 0;
}

} // End of anonymous namespace

void initShaders() {
	if (!compileShader()) {
		warning("Could not compile GLSL shaders");
		g_system->fatalError();
	}

	GLCALL(glUseProgram(s_shaderProgram));
}

void deinitShaders() {
	destroyShader();
}

void setProjectionMatrix(const GLfloat *projection) {
	GLCALL(glUniformMatrix4fv(s_projectionLocation, 1, GL_FALSE, projection));
}

} // End of namespace OpenGL

#endif
