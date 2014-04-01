#ifndef DIESEL_STATES_H
#define DIESEL_STATES_H

// Breathe headers
#include <breathe/gui/cManager.h>
#include <breathe/gui/cRenderer.h>
#include <breathe/render/cContext.h>
#include <breathe/render/cFont.h>
#include <breathe/render/cSystem.h>
#include <breathe/render/cVertexBufferObject.h>
#include <breathe/render/cWindow.h>
#include <breathe/util/cApplication.h>

// Diesel headers
#include "win32mmapplication.h"
#include "diesel.h"

namespace diesel
{
  class cApplication;

  // ** cState

  class cState : public breathe::util::cState
  {
  public:
    explicit cState(cApplication& application);
    virtual ~cState();

    void LoadResources();
    void DestroyResources();

  protected:
    breathe::gui::cStaticText* AddStaticText(breathe::gui::id_t id, const spitfire::string_t& sText, float x, float y, float width);
    breathe::gui::cRetroButton* AddRetroButton(breathe::gui::id_t id, const spitfire::string_t& sText, float x, float y, float width);
    breathe::gui::cRetroInput* AddRetroInput(breathe::gui::id_t id, const spitfire::string_t& sText, float x, float y, float width);
    breathe::gui::cRetroInputUpDown* AddRetroInputUpDown(breathe::gui::id_t id, int min, int max, int value, float x, float y, float width);
    breathe::gui::cRetroColourPicker* AddRetroColourPicker(breathe::gui::id_t id, float x, float y, float width);

    cApplication& application;

    cSettings& settings;

    breathe::render::cFont* pFont;

    breathe::audio::cManager* pAudioManager;

    breathe::gui::cManager* pGuiManager;
    breathe::gui::cRenderer* pGuiRenderer;

    breathe::gui::cLayer* pLayer;

    bool bIsWireframe;

  private:
    virtual void _OnEnter() override {}
    virtual void _OnExit() override {}
    virtual void _OnPause() override;
    virtual void _OnResume() override;

    virtual void _OnWindowEvent(const breathe::gui::cWindowEvent& event) override;
    virtual void _OnMouseEvent(const breathe::gui::cMouseEvent& event) override;
    virtual void _OnKeyboardEvent(const breathe::gui::cKeyboardEvent& event) override;

    virtual void _Update(const spitfire::math::cTimeStep& timeStep) override {}
    virtual void _UpdateInput(const spitfire::math::cTimeStep& timeStep) override {}
    virtual void _Render(const spitfire::math::cTimeStep& timeStep) override;

    virtual void _RenderToTexture(const spitfire::math::cTimeStep& timeStep) {}

    virtual void _OnStateCommandEvent(int iCommandID) {}
    virtual void _OnStateQuitEvent() {}
    virtual void _OnStateMouseEvent(const breathe::gui::cMouseEvent& event) {}
    virtual void _OnStateKeyboardEvent(const breathe::gui::cKeyboardEvent& event) {}

    virtual breathe::gui::EVENT_RESULT _OnWidgetEvent(const breathe::gui::cWidgetEvent& event) override { return breathe::gui::EVENT_RESULT::NOT_HANDLED_PERCOLATE; }

    void CreateVertexBufferObjectLetterBoxedRectangle(size_t width, size_t height);
    void DestroyVertexBufferObjectLetterBoxedRectangle();

    void CreateFrameBufferObjectLetterBoxedRectangle(size_t width, size_t height);
    void DestroyFrameBufferObjectLetterBoxedRectangle();

    void CreateShaderLetterBoxedRectangle();
    void DestroyShaderLetterBoxedRectangle();

    breathe::render::cVertexBufferObject* pVertexBufferObjectLetterBoxedRectangle;
    breathe::render::cTextureFrameBufferObject* pFrameBufferObjectLetterBoxedRectangle;
    breathe::render::cShader* pShaderLetterBoxedRectangle;
  };


  // ** States

  class cStatePhotoBrowser : public cState
  {
  public:
    explicit cStatePhotoBrowser(cApplication& application);

  private:
    void _Update(const spitfire::math::cTimeStep& timeStep);
    void _RenderToTexture(const spitfire::math::cTimeStep& timeStep);
    
    virtual void _OnStateCommandEvent(int iCommandID) override;
    virtual void _OnStateQuitEvent() override;
    virtual void _OnStateMouseEvent(const breathe::gui::cMouseEvent& event);
    virtual void _OnStateKeyboardEvent(const breathe::gui::cKeyboardEvent& event) override;

    breathe::gui::EVENT_RESULT _OnWidgetEvent(const breathe::gui::cWidgetEvent& event);

    struct OPTION {
      static const int NEW_GAME = 1;
      static const int HIGH_SCORES = 2;
      //static const int PREFERENCES = 3;
      static const int QUIT = 3;
    };

    bool bIsKeyReturn;

    bool bIsSinglePhotoMode;
  };

  class cStateAboutBox : public cState
  {
  public:
    explicit cStateAboutBox(cApplication& application);
    ~cStateAboutBox();

    void SetQuitSoon(); // Called by the pause menu when the game should quit

  private:
    void UpdateText();

    virtual void _OnStateKeyboardEvent(const breathe::gui::cKeyboardEvent& event) override;

    void _Update(const spitfire::math::cTimeStep& timeStep);
    void _UpdateInput(const spitfire::math::cTimeStep& timeStep);
    void _RenderToTexture(const spitfire::math::cTimeStep& timeStep);

    breathe::gui::cStaticText* pLevelText[4];
    breathe::gui::cStaticText* pScoreText[4];

    breathe::render::cTexture* pTextureBlock;

    breathe::render::cShader* pShaderBlock;

    bool bPauseSoon;
    bool bQuitSoon;
  };

  typedef cStateAboutBox cStateSettings;
  typedef cStateAboutBox cStateImportFromFolder;
}

#endif // DIESEL_STATES_H
