#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Xml.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"

// Settings
#include "VDSettings.h"
// Animation
#include "VDAnimation.h"
// Fbos
#include "VDFbo.h"

// Syphon
#if defined( CINDER_MAC )
#include "cinderSyphon.h"
#endif

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

namespace VideoDromm
{
	// stores the pointer to the VDMix instance
	typedef std::shared_ptr<class VDMix> 	VDMixRef;
	struct VDMixFbo
	{
		ci::gl::FboRef					fbo;
		ci::gl::Texture2dRef			texture;
		string							name;
	};
	class VDMix {
	public:
		VDMix(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation);
		static VDMixRef					create(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation)
		{
			return shared_ptr<VDMix>(new VDMix(aVDSettings, aVDAnimation));
		}
		void							update();
		void							updateAudio();
		void							resize();
		bool							handleMouseMove(MouseEvent &event);
		bool							handleMouseDown(MouseEvent &event);
		bool							handleMouseDrag(MouseEvent &event);
		bool							handleMouseUp(MouseEvent &event);
		bool							handleKeyDown(KeyEvent &event);
		bool							handleKeyUp(KeyEvent &event);
		bool							isFlipH() { return mFlipH; };
		bool							isFlipV() { return mFlipV; };

		unsigned int					getMixFbosCount() { return mMixFbos.size(); };
		string							getMixFboName(unsigned int aMixFboIndex);

		// blendmodes
		unsigned int					getFboBlendCount();
		ci::gl::TextureRef				getFboThumb(unsigned int aBlendIndex);
		void							useBlendmode(unsigned int aBlendIndex);
		// fbolist
		unsigned int					getFboListSize() { return mFboList.size(); };

		ci::gl::TextureRef				getMixTexture(unsigned int aMixFboIndex = 0);
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex = 0);
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex);
		unsigned int					getBlendFbosCount() { return mBlendFbos.size(); }

		// RTE in release mode ci::gl::Texture2dRef			getRenderedTexture(bool reDraw = true);
		ci::gl::Texture2dRef			getRenderTexture();
		void							save();
		void							load();
		// fbos
		unsigned int 					createShaderFbo(string aShaderFilename, unsigned int aInputTextureIndex = 0);
		unsigned int					createShaderFboFromString(string aFragmentShaderString, string aShaderFilename);
		string							getFboName(unsigned int aFboIndex) { return mFboList[aFboIndex]->getName(); };
		void							setFboInputTexture(unsigned int aFboIndex, unsigned int aInputTextureIndex);
		unsigned int					getFboInputTextureIndex(unsigned int aFboIndex);
		void							fboFlipV(unsigned int aFboIndex);
		bool							isFboFlipV(unsigned int aFboIndex);
		void							setFboFragmentShaderIndex(unsigned int aFboIndex, unsigned int aFboShaderIndex);
		unsigned int					getFboFragmentShaderIndex(unsigned int aFboIndex);
		unsigned int					getFboAIndex(unsigned int aIndex) { return 0; };
		unsigned int					getFboBIndex(unsigned int aIndex) { return 1; };

		// feedback get/set
		int								getFeedbackFrames() {
			return mFeedbackFrames;
		};
		void							setFeedbackFrames(int aFeedbackFrames) {
			mFeedbackFrames = aFeedbackFrames;
		};
		// textures
		ci::gl::TextureRef				getInputTexture(unsigned int aTextureIndex);
		string							getInputTextureName(unsigned int aTextureIndex);
		unsigned int					getInputTexturesCount();

		int								getInputTextureXLeft(unsigned int aTextureIndex);
		void							setInputTextureXLeft(unsigned int aTextureIndex, int aXLeft);
		int								getInputTextureYTop(unsigned int aTextureIndex);
		void							setInputTextureYTop(unsigned int aTextureIndex, int aYTop);
		int								getInputTextureXRight(unsigned int aTextureIndex);
		void							setInputTextureXRight(unsigned int aTextureIndex, int aXRight);
		int								getInputTextureYBottom(unsigned int aTextureIndex);
		void							setInputTextureYBottom(unsigned int aTextureIndex, int aYBottom);
		bool							isFlipVInputTexture(unsigned int aTextureIndex);
		bool							isFlipHInputTexture(unsigned int aTextureIndex);
		void							inputTextureFlipV(unsigned int aTextureIndex);
		void							inputTextureFlipH(unsigned int aTextureIndex);
		bool							getInputTextureLockBounds(unsigned int aTextureIndex);
		void							toggleInputTextureLockBounds(unsigned int aTextureIndex);
		unsigned int					getInputTextureOriginalWidth(unsigned int aTextureIndex);
		unsigned int					getInputTextureOriginalHeight(unsigned int aTextureIndex);
		void							togglePlayPause(unsigned int aTextureIndex);
		void							loadImageFile(string aFile, unsigned int aTextureIndex);
		void							loadAudioFile(string aFile);
		void							loadMovie(string aFile, unsigned int aTextureIndex);
		bool							loadImageSequence(string aFolder, unsigned int aTextureIndex);
		void							updateStream(string * aStringPtr);

		// movie
		bool							isMovie(unsigned int aTextureIndex);

		// sequence
		bool							isSequence(unsigned int aTextureIndex);
		bool							isLoadingFromDisk(unsigned int aTextureIndex);
		void							toggleLoadingFromDisk(unsigned int aTextureIndex);
		void							syncToBeat(unsigned int aTextureIndex);
		void							reverse(unsigned int aTextureIndex);
		float							getSpeed(unsigned int aTextureIndex);
		void							setSpeed(unsigned int aTextureIndex, float aSpeed);
		int								getPosition(unsigned int aTextureIndex);
		void							setPlayheadPosition(unsigned int aTextureIndex, int aPosition);
		int								getMaxFrame(unsigned int aTextureIndex);
		// shaders
		void							updateShaderThumbFile(unsigned int aShaderIndex);
		void							removeShader(unsigned int aShaderIndex);
		void							setFragmentShaderString(unsigned int aShaderIndex, string aFragmentShaderString, string aName = "");
		//string							getVertexShaderString(unsigned int aShaderIndex);
		string							getFragmentShaderString(unsigned int aShaderIndex);
		unsigned int					getShadersCount() { return mShaderList.size(); };
		string							getShaderName(unsigned int aShaderIndex);
		ci::gl::TextureRef				getShaderThumb(unsigned int aShaderIndex);
		string							getFragmentString(unsigned int aShaderIndex) { return mShaderList[aShaderIndex]->getFragmentString(); };
		// spout output
		void							toggleSharedOutput(unsigned int aMixFboIndex = 0);
		bool							isSharedOutputActive() { return mSharedOutputActive; };
		unsigned int					getSharedMixIndex() { return mSharedFboIndex; };

	private:
		bool							mFlipV;
		bool							mFlipH;
		std::string						mFbosPath;
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;

		//! mix shader
		gl::GlslProgRef					mMixShader;

		// Animation
		VDAnimationRef					mVDAnimation;
		// Settings
		VDSettingsRef					mVDSettings;

		//! Fbos
		map<int, VDMixFbo>				mMixFbos;
		map<int, VDMixFbo>				mTriangleFbos;
		// maintain a list of fbos specific to this mix
		VDFboList						mFboList;
		fs::path						mMixesFilepath;

		//! Shaders
		VDShaderList					mShaderList;
		void							initShaderList();
		//! Textures
		VDTextureList					mTextureList;
		fs::path						mTexturesFilepath;
		bool							initTextureList();
		// blendmodes fbos
		map<int, ci::gl::FboRef>		mBlendFbos;
		int								mCurrentBlend;
		gl::GlslProgRef					mGlslMix, mGlslBlend, mGlslFeedback;
		// render
		void							renderMix();
		void							renderBlend();
		// warping
		gl::FboRef						mRenderFbo;
		// warp rendered texture
		ci::gl::Texture2dRef			mRenderedTexture;
		// feedback
		// 0: only last rendered 1+: number of feedback images
		unsigned int					mFeedbackFrames;
		map<int, ci::gl::Texture2dRef>	mOutputTextures;
		unsigned int					mCurrentFeedbackIndex;
		gl::FboRef						mFeedbackFbo;

		// Output texture
		ci::gl::Texture2dRef			mFeedbackTexture;
		// shared texture output
		bool							mSharedOutputActive;
		unsigned int					mSharedFboIndex;
		bool							mSpoutInitialized;
		char							mSenderName[256];
#if defined( CINDER_MSW )
		// spout output
		SpoutSender						mSpoutSender;
#endif
		// syphon output
#if defined( CINDER_MAC )
		syphonServer                    mSyphonServer;
#endif
	};
}
