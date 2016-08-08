#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Animation
#include "VDAnimation.h"

#include "VDTexture.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

class VDApp : public App {

public:
	void						setup() override;
	void						mouseDown( MouseEvent event ) override;
	void						mouseDrag(MouseEvent event) override;
	void						update() override;
	void						draw() override;
	void						fileDrop(FileDropEvent event) override;
	void						cleanup() override;
	void						resize() override;

private:
	// Settings
	VDSettingsRef				mVDSettings;
	// Session
	VDSessionRef				mVDSession;
	// Log
	VDLogRef					mVDLog;
	// Animation
	VDAnimationRef				mVDAnimation;

	VDTextureList				mTexs;
	fs::path					mTexturesFilepath;
	int							i, x;
	ci::gl::FboRef				mFboA, mFboB, mFboMix;
	vector<ci::gl::FboRef>		mFboBlend;
	gl::GlslProgRef				mGlslA, mGlslB, mGlslMix, mGlslBlend;
	void						renderSceneA();
	void						renderSceneB();
	void						renderMix();
	void						renderBlend();
	int							mCurrentBlend;
};


void VDApp::setup()
{
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDSession);
	// initialize
#if (defined(  CINDER_MSW) )
	mTexturesFilepath = getAssetPath("") / "defaulttextures.xml";
#else
	mTexturesFilepath = getAssetPath("") / "defaulttexturesquicktime.xml";
#endif
	if (fs::exists(mTexturesFilepath)) {
		// load textures from file if one exists
		mTexs = VDTexture::readSettings(mVDAnimation, loadFile(mTexturesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mTexs.push_back(TextureAudio::create(mVDAnimation));
	}
	gl::Texture::Format fmt;
	fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
	fmt.setBorderColor(Color::black());

	gl::Fbo::Format fboFmt;
	fboFmt.setColorTextureFormat(fmt);
	mFboA = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
	mFboB = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
	mFboMix = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
	for (size_t i = 0; i < 27; i++)
	{
		mFboBlend.push_back(gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt));

	}
	mCurrentBlend = 0;
	mGlslA = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadAsset("simple330a.frag"));
	mGlslB = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadAsset("simple330b.frag"));
	mGlslMix = gl::GlslProg::create(loadAsset("passthru.vert"), loadAsset("mix.frag"));
	mGlslBlend = gl::GlslProg::create(loadAsset("passthru.vert"), loadAsset("mix.frag"));
	gl::enableDepthRead();
	gl::enableDepthWrite();
}
void VDApp::fileDrop(FileDropEvent event)
{
	int index = 1;
	string ext = "";
	// use the last of the dropped files
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");
	bool found = false;

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);

	if (ext == "wav" || ext == "mp3")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::AUDIO) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "png" || ext == "jpg")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::IMAGE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "mov")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::MOVIE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "frag")
	{
		if (event.getX() < getWindowWidth() / 2) {
			mGlslA = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadFile(mFile));
		}
		else {
			mGlslB = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadFile(mFile));

		}
	}
	else if (ext == "")
	{
		// try loading image sequence from dir
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::IMAGESEQUENCE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}

}
void VDApp::resize()
{

}
void VDApp::update()
{

}
void VDApp::cleanup()
{
	// save textures
	VDTexture::writeSettings(mTexs, writeFile(mTexturesFilepath));

	quit();
}
void VDApp::mouseDown(MouseEvent event)
{
	for (auto tex : mTexs)
	{
		tex->setXLeft(event.getX());
		tex->setYTop(event.getY());
	}
}
void VDApp::mouseDrag(MouseEvent event)
{
	for (auto tex : mTexs)
	{
		tex->setXRight(event.getX());
		tex->setYBottom(event.getY());
	}
}

void VDApp::update()
{
	mGlslA->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlslA->uniform("iChannel0", 0);
	mGlslB->uniform("iGlobalTime", (float)(getElapsedSeconds()/2));
	mGlslB->uniform("iChannel0", 0);

	mVDSettings->iBlendMode = getElapsedFrames()/48 % 27;
	mGlslMix->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlslMix->uniform("iResolution", vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0));
	mGlslMix->uniform("iChannelResolution", mVDSettings->iChannelResolution, 4);
	mGlslMix->uniform("iMouse", vec4(mVDSettings->mRenderPosXY.x, mVDSettings->mRenderPosXY.y, mVDSettings->iMouse.z, mVDSettings->iMouse.z));//iMouse =  Vec3i( event.getX(), mRenderHeight - event.getY(), 1 );
	mGlslMix->uniform("iChannel0", 0);
	mGlslMix->uniform("iChannel1", 1);
	mGlslMix->uniform("iAudio0", 0);
	mGlslMix->uniform("iFreq0", mVDAnimation->iFreqs[0]);
	mGlslMix->uniform("iFreq1", mVDAnimation->iFreqs[1]);
	mGlslMix->uniform("iFreq2", mVDAnimation->iFreqs[2]);
	mGlslMix->uniform("iFreq3", mVDAnimation->iFreqs[3]);
	mGlslMix->uniform("iChannelTime", mVDSettings->iChannelTime, 4);
	mGlslMix->uniform("iColor", vec3(mVDAnimation->controlValues[1], mVDAnimation->controlValues[2], mVDAnimation->controlValues[3]));// mVDSettings->iColor);
	mGlslMix->uniform("iBackgroundColor", vec3(mVDAnimation->controlValues[5], mVDAnimation->controlValues[6], mVDAnimation->controlValues[7]));// mVDSettings->iBackgroundColor);
	mGlslMix->uniform("iSteps", (int)mVDAnimation->controlValues[20]);
	mGlslMix->uniform("iRatio", mVDAnimation->controlValues[11]);//check if needed: +1;//mVDSettings->iRatio);
	mGlslMix->uniform("width", 1);
	mGlslMix->uniform("height", 1);
	mGlslMix->uniform("iRenderXY", mVDSettings->mRenderXY);
	mGlslMix->uniform("iZoom", mVDAnimation->controlValues[22]);
	mGlslMix->uniform("iAlpha", mVDAnimation->controlValues[4] * mVDSettings->iAlpha);
	mGlslMix->uniform("iBlendmode", mVDSettings->iBlendMode);
	mGlslMix->uniform("iChromatic", mVDAnimation->controlValues[10]);
	mGlslMix->uniform("iRotationSpeed", mVDAnimation->controlValues[19]);
	mGlslMix->uniform("iCrossfade", 0.5f);// mVDAnimation->controlValues[18]);
	mGlslMix->uniform("iPixelate", mVDAnimation->controlValues[15]);
	mGlslMix->uniform("iExposure", mVDAnimation->controlValues[14]);
	mGlslMix->uniform("iDeltaTime", mVDAnimation->iDeltaTime);
	mGlslMix->uniform("iFade", (int)mVDSettings->iFade);
	mGlslMix->uniform("iToggle", (int)mVDAnimation->controlValues[46]);
	mGlslMix->uniform("iLight", (int)mVDSettings->iLight);
	mGlslMix->uniform("iLightAuto", (int)mVDSettings->iLightAuto);
	mGlslMix->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
	mGlslMix->uniform("iTransition", mVDSettings->iTransition);
	mGlslMix->uniform("iAnim", mVDSettings->iAnim.value());
	mGlslMix->uniform("iRepeat", (int)mVDSettings->iRepeat);
	mGlslMix->uniform("iVignette", (int)mVDAnimation->controlValues[47]);
	mGlslMix->uniform("iInvert", (int)mVDAnimation->controlValues[48]);
	mGlslMix->uniform("iDebug", (int)mVDSettings->iDebug);
	mGlslMix->uniform("iShowFps", (int)mVDSettings->iShowFps);
	mGlslMix->uniform("iFps", mVDSettings->iFps);
	mGlslMix->uniform("iTempoTime", mVDAnimation->iTempoTime);
	mGlslMix->uniform("iGlitch", (int)mVDAnimation->controlValues[45]);
	mGlslMix->uniform("iTrixels", mVDAnimation->controlValues[16]);
	mGlslMix->uniform("iGridSize", mVDAnimation->controlValues[17]);
	mGlslMix->uniform("iBeat", mVDSettings->iBeat);
	mGlslMix->uniform("iSeed", mVDSettings->iSeed);
	mGlslMix->uniform("iRedMultiplier", mVDSettings->iRedMultiplier);
	mGlslMix->uniform("iGreenMultiplier", mVDSettings->iGreenMultiplier);
	mGlslMix->uniform("iBlueMultiplier", mVDSettings->iBlueMultiplier);
	mGlslMix->uniform("iFlipH", 0);
	mGlslMix->uniform("iFlipV", 0);
	mGlslMix->uniform("iParam1", mVDSettings->iParam1);
	mGlslMix->uniform("iParam2", mVDSettings->iParam2);
	mGlslMix->uniform("iXorY", mVDSettings->iXorY);
	mGlslMix->uniform("iBadTv", mVDSettings->iBadTv);

	mGlslBlend->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlslBlend->uniform("iResolution", vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0));
	mGlslBlend->uniform("iChannelResolution", mVDSettings->iChannelResolution, 4);
	mGlslBlend->uniform("iMouse", vec4(mVDSettings->mRenderPosXY.x, mVDSettings->mRenderPosXY.y, mVDSettings->iMouse.z, mVDSettings->iMouse.z));//iMouse =  Vec3i( event.getX(), mRenderHeight - event.getY(), 1 );
	mGlslBlend->uniform("iChannel0", 0);
	mGlslBlend->uniform("iChannel1", 1);
	mGlslBlend->uniform("iAudio0", 0);
	mGlslBlend->uniform("iFreq0", mVDAnimation->iFreqs[0]);
	mGlslBlend->uniform("iFreq1", mVDAnimation->iFreqs[1]);
	mGlslBlend->uniform("iFreq2", mVDAnimation->iFreqs[2]);
	mGlslBlend->uniform("iFreq3", mVDAnimation->iFreqs[3]);
	mGlslBlend->uniform("iChannelTime", mVDSettings->iChannelTime, 4);
	mGlslBlend->uniform("iColor", vec3(mVDAnimation->controlValues[1], mVDAnimation->controlValues[2], mVDAnimation->controlValues[3]));// mVDSettings->iColor);
	mGlslBlend->uniform("iBackgroundColor", vec3(mVDAnimation->controlValues[5], mVDAnimation->controlValues[6], mVDAnimation->controlValues[7]));// mVDSettings->iBackgroundColor);
	mGlslBlend->uniform("iSteps", (int)mVDAnimation->controlValues[20]);
	mGlslBlend->uniform("iRatio", mVDAnimation->controlValues[11]);//check if needed: +1;//mVDSettings->iRatio);
	mGlslBlend->uniform("width", 1);
	mGlslBlend->uniform("height", 1);
	mGlslBlend->uniform("iRenderXY", mVDSettings->mRenderXY);
	mGlslBlend->uniform("iZoom", mVDAnimation->controlValues[22]);
	mGlslBlend->uniform("iAlpha", mVDAnimation->controlValues[4] * mVDSettings->iAlpha);
	mGlslBlend->uniform("iBlendmode", mCurrentBlend);
	mGlslBlend->uniform("iChromatic", mVDAnimation->controlValues[10]);
	mGlslBlend->uniform("iRotationSpeed", mVDAnimation->controlValues[19]);
	mGlslBlend->uniform("iCrossfade", 0.5f);// mVDAnimation->controlValues[18]);
	mGlslBlend->uniform("iPixelate", mVDAnimation->controlValues[15]);
	mGlslBlend->uniform("iExposure", mVDAnimation->controlValues[14]);
	mGlslBlend->uniform("iDeltaTime", mVDAnimation->iDeltaTime);
	mGlslBlend->uniform("iFade", (int)mVDSettings->iFade);
	mGlslBlend->uniform("iToggle", (int)mVDAnimation->controlValues[46]);
	mGlslBlend->uniform("iLight", (int)mVDSettings->iLight);
	mGlslBlend->uniform("iLightAuto", (int)mVDSettings->iLightAuto);
	mGlslBlend->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
	mGlslBlend->uniform("iTransition", mVDSettings->iTransition);
	mGlslBlend->uniform("iAnim", mVDSettings->iAnim.value());
	mGlslBlend->uniform("iRepeat", (int)mVDSettings->iRepeat);
	mGlslBlend->uniform("iVignette", (int)mVDAnimation->controlValues[47]);
	mGlslBlend->uniform("iInvert", (int)mVDAnimation->controlValues[48]);
	mGlslBlend->uniform("iDebug", (int)mVDSettings->iDebug);
	mGlslBlend->uniform("iShowFps", (int)mVDSettings->iShowFps);
	mGlslBlend->uniform("iFps", mVDSettings->iFps);
	mGlslBlend->uniform("iTempoTime", mVDAnimation->iTempoTime);
	mGlslBlend->uniform("iGlitch", (int)mVDAnimation->controlValues[45]);
	mGlslBlend->uniform("iTrixels", mVDAnimation->controlValues[16]);
	mGlslBlend->uniform("iGridSize", mVDAnimation->controlValues[17]);
	mGlslBlend->uniform("iBeat", mVDSettings->iBeat);
	mGlslBlend->uniform("iSeed", mVDSettings->iSeed);
	mGlslBlend->uniform("iRedMultiplier", mVDSettings->iRedMultiplier);
	mGlslBlend->uniform("iGreenMultiplier", mVDSettings->iGreenMultiplier);
	mGlslBlend->uniform("iBlueMultiplier", mVDSettings->iBlueMultiplier);
	mGlslBlend->uniform("iFlipH", 0);
	mGlslBlend->uniform("iFlipV", 0);
	mGlslBlend->uniform("iParam1", mVDSettings->iParam1);
	mGlslBlend->uniform("iParam2", mVDSettings->iParam2);
	mGlslBlend->uniform("iXorY", mVDSettings->iXorY);
	mGlslBlend->uniform("iBadTv", mVDSettings->iBadTv);
	renderSceneA();
	renderSceneB();
	renderMix();
	renderBlend();
}
void VDApp::renderSceneA()
{
	gl::ScopedFramebuffer scopedFbo(mFboA);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslA);
	mTexs[1]->getTexture()->bind(0);

	gl::drawSolidRect(Rectf(0, 0, mFboA->getWidth(), mFboA->getHeight()));
}
void VDApp::renderSceneB()
{
	gl::ScopedFramebuffer scopedFbo(mFboB);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslB);
	mTexs[2]->getTexture()->bind(0);

	gl::drawSolidRect(Rectf(0, 0, mFboB->getWidth(), mFboB->getHeight()));
}
void VDApp::renderMix()
{
	gl::ScopedFramebuffer scopedFbo(mFboMix);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslMix);
	mFboA->getColorTexture()->bind(0);
	mFboB->getColorTexture()->bind(1);
	gl::drawSolidRect(Rectf(0, 0, mFboMix->getWidth(), mFboMix->getHeight()));
}
void VDApp::renderBlend()
{
	mCurrentBlend++;
	if (mCurrentBlend>mFboBlend.size() - 1) mCurrentBlend = 0;
	gl::ScopedFramebuffer scopedFbo(mFboBlend[mCurrentBlend]);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslBlend);
	mFboA->getColorTexture()->bind(0);
	mFboB->getColorTexture()->bind(1);
	gl::drawSolidRect(Rectf(0, 0, mFboBlend[mCurrentBlend]->getWidth(), mFboBlend[mCurrentBlend]->getHeight()));
}
void VDApp::draw()
{
	gl::clear(Color::black());
	i = 0;
	for (auto tex : mTexs)
	{
		int x = 128 * i;
		gl::draw(tex->getTexture(), Rectf(0 + x, 0, 128 + x, 128));
		i++;
	}
	for (size_t b = 0; b < mFboBlend.size() - 1; b++)
	{
		int x = 64 * b;
		gl::draw(mFboBlend[b]->getColorTexture(), Rectf(0 + x, 256, 64 + x, 320));
	}
	gl::draw(mFboA->getColorTexture(), Rectf(0, 128, 128, 256));
	gl::draw(mFboMix->getColorTexture(), Rectf(128, 128, 256, 256));
	gl::draw(mFboB->getColorTexture(), Rectf(384, 128, 512, 256));
}

CINDER_APP(VDApp, RendererGl)
