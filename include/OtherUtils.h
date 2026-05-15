#include "syati.h"
#include "PictureBookLayout.h"

class LayoutManager {
public:
    nw4r::lyt::Pane* getPane(const char*) const;
    LayoutPaneCtrl* getPaneCtrl(const char*) const;
};

class TextBoxRecursiveOperation {
public:
    /// @brief Destroys the `TextBoxRecursiveOperation`.
    virtual ~TextBoxRecursiveOperation() {}

    virtual void execute(nw4r::lyt::TextBox* pTextBox) const = 0;
};

class TextBoxRecursiveSetVerticalPosition : public TextBoxRecursiveOperation {
public:
    /// @brief Creates a new `TextBoxRecursiveSetVerticalPosition`.
    TextBoxRecursiveSetVerticalPosition(u8 position) :
        mPosition(position)
    {}

    virtual void execute(nw4r::lyt::TextBox* pTextBox) const;

private:
    /* 0x4 */ u8 mPosition;
};

class TextBoxRecursiveSetHorizontalPosition : public TextBoxRecursiveOperation {
public:
    /// @brief Creates a new `TextBoxRecursiveSetHorizontalPosition`.
    TextBoxRecursiveSetHorizontalPosition(u8 position) :
        mPosition(position)
    {}

    virtual void execute(nw4r::lyt::TextBox* pTextBox) const {
        u8 position = this->mPosition;
    
        u8 currentState = *((u8*)pTextBox + 0xFC);
        u8 preservedFlagsQuotient = currentState / 3;
        u8 newState = (preservedFlagsQuotient * 3) + position;
        
        *((u8*)pTextBox + 0xFC) = newState;
    }

private:
    /* 0x4 */ u8 mPosition;
};

enum StarPointerModeEnum {};

class StarPointerOnOffController : public NerveExecutor {
public:
    void requestMode(void*, StarPointerModeEnum);
};

class LayoutPaneCtrl {
public:
    void start(const char*, u32);
    J3DFrameCtrl* getFrameCtrl(u32);
};

extern "C" {
    StarPointerOnOffController* __kAutoMap_8005BCB0();  // getStarPointerOnOffController()
    GameScene* __kAutoMap_80452A70();   // getGameScene()
}

namespace MR
{
    void createLytTextMap(const char*, const char*);
    bool isExistGameMessage(const char*);
    void pauseOnCameraDirector();
    nw4r::lyt::TexMap* createLytTexMap(const char*, const char*);
    void replacePaneTexture(LayoutActor*, const char*, const nw4r::lyt::TexMap*, u8);
    void activateGameSceneDraw3D() {
        __kAutoMap_80452A70()->mDraw3D = true;
    }
    void deactivateGameSceneDraw3D() {
        __kAutoMap_80452A70()->mDraw3D = false; 
    }
    bool isExistResourceInArc(const char* pArchive, const char* pFile) {
        char path[64];
        snprintf(path, sizeof(path), "/ObjectData/%s", pArchive);
        JKRArchive* archive = (JKRArchive*)MR::mountArchive(path, nullptr, false);
        void* resource = archive->getResource(pFile);
        return resource != nullptr;

        /* ResourceHolder* holder = createAndAddResourceHolder(pArchive);
        return holder->mFileInfoTable->isExistRes(pFile); */
    }
    void requestStarPointerModePictureBook(void* v) {
        StarPointerOnOffController* pController = __kAutoMap_8005BCB0();
        pController->requestMode(v, (StarPointerModeEnum)7);  // Surprisignly, the code for this was left at SMG2. Idk if just left or if it's used somewhere else.
    }
    f32 calcNerveRate(const LayoutActor* pActor, s32 stepMin, s32 stepMax) {
        return clamp(normalize(pActor->getNerveStep(), stepMin, stepMax), 0.0f, 1.0f);
    }
    f32 calcNerveRate(const LayoutActor* pActor, s32 stepMax) {
        return stepMax <= 0 ? 1.0f : clamp(static_cast<f32>(pActor->getNerveStep()) / stepMax, 0.0f, 1.0f);
    }
    void setPaneAlphaFloat(const LayoutActor* pActor, const char* pPaneName, f32 num) {
        f32 var = clamp(num, 0.0f, 1.0f);
        nw4r::lyt::Pane* pane = pActor->getLayoutManager()->getPane(pPaneName);
        *((u8 *)pane + 0xB8) = var * 255;  // pane->mAlpha = var * 255
    }
    void invalidateAppearance(ButtonPaneController* pButton) {
        pButton->mAnimNameAppear = nullptr;
        pButton->mAnimNameEnd = nullptr;
        pButton->_1C = false;
    }
    void initFrameCtrlReverse(J3DFrameCtrl* pFrameCtrl) {
        pFrameCtrl->mAttribute = pFrameCtrl->EMode_RESET;
        pFrameCtrl->setRate(-pFrameCtrl->mRate);
        pFrameCtrl->setFrame(pFrameCtrl->mEnd);
    }
    void startAnimReverseOneTime(LayoutActor* pActor, const char* pAnimName, u32 animLayer) {
        LayoutPaneCtrl* pPaneCtrl = pActor->getLayoutManager()->getPaneCtrl(nullptr);

        pPaneCtrl->start(pAnimName, animLayer);
        initFrameCtrlReverse(pPaneCtrl->getFrameCtrl(animLayer));
    }
}

namespace Other {
    void openWithTurn(IconAButton* pAButton) {
        pAButton->appear();
        pAButton->updateFollowPos();
        MR::showPane(pAButton, "PicPlate");
        pAButton->setNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance);
        MR::setTextBoxGameMessageRecursive(pAButton, "IconAButton", "Layout_SystemTurn");
    }
    void setTextBoxHorizontalPositionRecursive(LayoutActor* pActor, const char* pPaneName, u8 position) {
        MR::executeTextBoxRecursive(pActor, pPaneName, TextBoxRecursiveSetHorizontalPosition(position));
    }
    void setTextBoxVerticalPositionRecursive(LayoutActor* pActor, const char* pPaneName, u8 position) {
        MR::executeTextBoxRecursive(pActor, pPaneName, TextBoxRecursiveSetVerticalPosition(position));
    }
}