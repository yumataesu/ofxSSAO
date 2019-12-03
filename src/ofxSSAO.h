#pragma once

#ifndef ofxSSAO_h
#define ofxSSAO_h

#include "ofFbo.h"
#include "ofTexture.h"
#include "ofVboMesh.h"
#include "ofxAutoReloadedShader.h"
#define _USE_MATH_DEFINES
#include<math.h>

namespace ofx {
class SSAO
{
public:
	SSAO(int width, int height) 
	{
        std::string ssao_path = "../../addons/ofxSSAO/assets/ssao";
        std::string blur_path = "../../addons/ofxSSAO/assets/ssao";
        
		if (!ssao_shader_.load(ssao_path)) { //of addon path
			ssao_shader_.load("../../" + ssao_path); // of local addon path
		}
		if (!blur_shader_.load(blur_path)) {
			blur_shader_.load("../../" + blur_path);
		}

        ofFbo::Settings settings;
	    settings.width = width;
	    settings.height = height;
	    settings.useDepth = true;
	    settings.useStencil = true;
	    settings.depthStencilAsTexture = true;
	    settings.textureTarget = GL_TEXTURE_2D;
	    settings.internalformat = GL_RGBA32F;
	    settings.wrapModeVertical = GL_CLAMP;
	    settings.wrapModeHorizontal = GL_CLAMP;
	    settings.minFilter = GL_NEAREST;
	    settings.maxFilter = GL_NEAREST;

		quad_.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
		quad_.addVertex(ofVec3f(1.0, 1.0, 0.0));
		quad_.addTexCoord(ofVec2f(1.0f, 1.0f));
		quad_.addVertex(ofVec3f(1.0, -1.0, 0.0));
		quad_.addTexCoord(ofVec2f(1.0f, 0.0f));
		quad_.addVertex(ofVec3f(-1.0, -1.0, 0.0));
		quad_.addTexCoord(ofVec2f(0.0f, 0.0f));
		quad_.addVertex(ofVec3f(-1.0, 1.0, 0.0));
		quad_.addTexCoord(ofVec2f(0.0f, 1.0f));

		ofDisableArbTex();
		ssao_fbo_.allocate(settings);
		result_fbo_.allocate(settings);
		ofEnableArbTex();

		noiseTexture = genNoiseTexture();
	}

	void process(
		const ofTexture& position_tex,
		const ofTexture& normal_tex, 
		const glm::mat4& view, 
		const glm::mat4& projection) 
	{

		ssao_fbo_.begin();
		ssao_shader_.begin();
		ssao_shader_.setUniformTexture("gPosition", position_tex, 0);
		ssao_shader_.setUniformTexture("gNormal", normal_tex, 1);
		ssao_shader_.setUniformTexture("noiseTexture", GL_TEXTURE_2D, noiseTexture, 2);
		ssao_shader_.setUniform3fv("samples", &ssaoKernel[0].x, KARNEL_SAMPLE);
		ssao_shader_.setUniformMatrix4f("projection", projection);
		ssao_shader_.setUniformMatrix4f("view", view);
		quad_.draw();
		ssao_shader_.end();
		ssao_fbo_.end();

		result_fbo_.begin();
		blur_shader_.begin();
		ofClear(0.0f);
		blur_shader_.setUniformTexture("ssao", ssao_fbo_.getTexture(), 0);
		quad_.draw();
		blur_shader_.end();
		result_fbo_.end();
	}

	ofTexture& getTexture() { return result_fbo_.getTexture(); }

	virtual void drawGui() {}


private:
	ofFbo ssao_fbo_, result_fbo_;
	ofxAutoReloadedShader ssao_shader_;
	ofxAutoReloadedShader blur_shader_;
	ofVboMesh quad_;

	const int KARNEL_SAMPLE = 64;
	std::vector<ofVec3f> ssaoKernel;
	GLuint noiseTexture;

	GLuint genNoiseTexture() {

		auto lerp = [&](float a, float b, float f) {
			return a + f * (b - a);
		};

		// generate sample kernel
		// ----------------------
		ssaoKernel.resize(KARNEL_SAMPLE);
		float randomFloats = ofRandom(1.0);
		for (int i = 0; i < KARNEL_SAMPLE; i++) {
			glm::vec3 sample = glm::vec3(ofRandom(1.0) * 2.0 - 1.0, ofRandom(1.0) * 2.0 - 1.0, ofRandom(1.0));
			sample = glm::normalize(sample);
			sample *= randomFloats;
			float scale = float(i) / 64.0;

			// Scale samples s.t. they're more aligned to center of kernel
			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			ssaoKernel[i] = sample;
		}


		// generate noise texture
		// ----------------------
		GLuint noiseTexture;
		vector<glm::vec3> ssaoNoise;
		for (int i = 0; i < 16; i++) {
			glm::vec3 noise = ofVec3f(ofRandom(1.0) * 2.0 - 1.0, ofRandom(1.0) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
			ssaoNoise.emplace_back(noise);
		}
		glGenTextures(1, &noiseTexture);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		return noiseTexture;
	}
};
}

#endif
