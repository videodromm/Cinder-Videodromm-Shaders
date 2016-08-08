// From https://github.com/ulyssesp/oschader-cinder
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include <regex>

#include "Osc.h"

#include "AudioSource.h"
#include "InputState.h"
#include "ProgramFactory.h"
#include "ProgramState.h"
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

class _TBOX_PREFIX_App : public App {

public:
	void						setup() override;
	void						mouseDown( MouseEvent event ) override;
	void						mouseDrag(MouseEvent event) override;
	void						update() override;
	void						draw() override;
	void						fileDrop(FileDropEvent event) override;
	void						cleanup() override;
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
	std::shared_ptr<osc::ReceiverUdp> mOscReceiver;

	ci::gl::FboRef a, b;

	std::shared_ptr<ProgramState> mState;
	ProgramFactory mFactory;
	AudioSource mAudioSource;
};


void _TBOX_PREFIX_App::setup()
{
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDSession);


	mState = std::make_shared<ProgramState>();
	mFactory.setup(mState);

	mAudioSource.setup();

	mOscReceiver = std::shared_ptr<osc::ReceiverUdp>(new osc::ReceiverUdp(9001));
	mOscReceiver->bind();
	mOscReceiver->listen();

	mOscReceiver->setListener("/progs/effect", [&](const osc::Message msg) {
		ProgramRef s = mState->getProgram(msg.getArgString(0));
		if (s) {
			s->setEffect(msg.getArgString(1));
		}
	});

	mOscReceiver->setListener("/progs/effect/clear", [&](const osc::Message msg) {
		ProgramRef s = mState->getProgram(msg.getArgString(0));
		if (s) {
			s->clearEffect();
		}
	});

	mOscReceiver->setListener("/progs/combinator", [&](const osc::Message msg) {
		ProgramRef s = mState->getProgram(msg.getArgString(0));
		if (s) {
			s->setConnection(msg.getArgString(1));
		}
	});

	mOscReceiver->setListener("/progs", [&](const osc::Message msg) {
		mState->setProgram(msg.getArgString(0), msg.getArgString(1), std::bind(&ProgramFactory::createProgram, mFactory, msg.getArgString(1)));
	});

	mOscReceiver->setListener("/progs/uniform", [&](const osc::Message msg) {
		ProgramRef p = mState->getProgram(msg.getArgString(0));
		if (p) {
			if(msg.getArgType(2) == osc::ArgType::FLOAT) {
				p->updateUniform(msg.getArgString(1), msg.getArgFloat(2));
			}
			else if (msg.getArgType(2) == osc::ArgType::STRING) {
				p->updateUniform(msg.getArgString(1), msg.getArgString(2), msg.getArgFloat(3));
			}
		}
	});

	gl::Texture::Format fmt;
	fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
	fmt.setBorderColor(Color::black());

	gl::Fbo::Format fboFmt;
	fboFmt.setColorTextureFormat(fmt);
	a = gl::Fbo::create(app::getWindowWidth(), app::getWindowHeight(), fboFmt);
	b = gl::Fbo::create(app::getWindowWidth(), app::getWindowHeight(), fboFmt);
}
void _TBOX_PREFIX_App::fileDrop(FileDropEvent event)
{
	int index = (int)(event.getX() / (mVDSettings->uiElementWidth + mVDSettings->uiMargin));
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();

}
void _TBOX_PREFIX_App::update()
{
	input::InputState is;
	is.audioTexture = mAudioSource.getMagSpectrumTexture();
	is.volume = mAudioSource.getVolume();
	is.eqTexture = std::bind(&AudioSource::getEqTexture, mAudioSource, std::placeholders::_1);

	mState->update([is](std::shared_ptr<Program> prog) {
		prog->update(is);
	});
}
void _TBOX_PREFIX_App::cleanup()
{
	// save textures
	VDTexture::writeSettings(mTexs, writeFile(mTexturesFilepath));

	quit();
}
void _TBOX_PREFIX_App::mouseDown(MouseEvent event)
{
	for (auto tex : mTexs)
	{
		tex->setXLeft(event.getX());
		tex->setYTop(event.getY());
	}
}
void _TBOX_PREFIX_App::mouseDrag(MouseEvent event)
{
	for (auto tex : mTexs)
	{
		tex->setXRight(event.getX());
		tex->setYBottom(event.getY());
	}
}

void _TBOX_PREFIX_App::draw()
{
	gl::clear( Color::black() );

	ProgramRef s = mState->getProgram("s");
	if (s) {
		gl::draw(s->getColorTexture(a, b));
	}
}


CINDER_APP(_TBOX_PREFIX_App, RendererGl(), [&](App::Settings *settings) {
	FullScreenOptions options;
	std::vector<DisplayRef> displays = Display::getDisplays();
	//settings->setFullScreen(true, options);
	settings->setWindowSize(displays[0]->getSize());
	if (displays.size() > 1) {
		settings->setDisplay(displays[1]);
	}
	settings->setFrameRate(60.0f);
})
