#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

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
private:
	VDTextureList				mTexs;
	fs::path					mTexturesFilepath;
	int							i, x;
};


void VDApp::setup()
{

#if (defined(  CINDER_MSW) )
	mTexturesFilepath = getAssetPath("") / "defaulttextures.xml";
#else
	mTexturesFilepath = getAssetPath("") / "defaulttexturesquicktime.xml";
#endif
	if (fs::exists(mTexturesFilepath)) {
		// load textures from file if one exists
		mTexs = VDTexture::readSettings(loadFile(mTexturesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mTexs.push_back(TextureAudio::create());
	}
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

void VDApp::draw()
{
	gl::clear( Color::black() );
	i = 0;
	for (auto tex : mTexs)
	{
		int x = 128 * i;
		gl::draw(tex->getTexture(), Rectf(0 + x, 0, 128 + x, 128));
		i++;
	}
}


CINDER_APP(VDApp, RendererGl)