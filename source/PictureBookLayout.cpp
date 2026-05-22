#include "PictureBookCloseButton.h"
#include "PictureBookLayout.h"
#include "OtherUtils.h"

namespace {
    const s32 cBookOpenFrame = 60;
    // const s32 cBookCloseFrame
    const s32 cFadeFrame = 60;
    const s32 cWaitNoTextFrame = 60;
    const s32 cFadeTextFrame = 30;
    // const s32 cReadedSpeedRate
    const s32 cPageNextEndNormalSeStep = 81;
    const s32 cPageNextEndFastSeStep = 27;
    const char* cContentsPaneName[] = {
        "Chapter1",
        "Chapter2",
        "Chapter3",
        "Chapter4",
        "Chapter5",
        "Chapter6",
        "Chapter7",
        "Chapter8",
        "Chapter9",
    };
    const char* cContentsPointingPaneName[] = {
        "BoxButton1",
        "BoxButton2",
        "BoxButton3",
        "BoxButton4",
        "BoxButton5",
        "BoxButton6",
        "BoxButton7",
        "BoxButton8",
        "BoxButton9",
    };

    s32 getTextureNum(s32 chapterNo) {
        s32 textureNo = 0;

        char chapterArcName[64];
        snprintf(chapterArcName, sizeof(chapterArcName), "PictureBookChapter%d.arc", chapterNo);

        for (;;) {
            char pageTexName[64];
            snprintf(pageTexName, sizeof(pageTexName), "Chapter%dPage%d.bti", chapterNo, textureNo + 1);

            if (!MR::isExistResourceInArc(chapterArcName, pageTexName)) {
                break;
            }

            textureNo++;
        }

        return textureNo;
    }

    s32 getPageNum(s32 chapterNo) {
        for (s32 pageNo = 0; pageNo < 100; pageNo++) {
            char messageId[64];
            snprintf(messageId, sizeof(messageId), "PictureBookChapter%d_Page%d_%03d", chapterNo, pageNo + 1, 0);

            if (MR::isExistGameMessage(messageId)) {
                continue;
            }

            return pageNo;
        }

        return 1;
    }

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

namespace NrvPictureBookLayout {
    FULL_NERVE(PictureBookLayoutNrvOpen, PictureBookLayout, Open);
    FULL_NERVE(PictureBookLayoutNrvContentsSelect, PictureBookLayout, ContentsSelect);
    FULL_NERVE(PictureBookLayoutNrvContentsFadeOut, PictureBookLayout, ContentsFadeOut);
    FULL_NERVE(PictureBookLayoutNrvFadeIn, PictureBookLayout, FadeIn);
    FULL_NERVE(PictureBookLayoutNrvWaitNoText, PictureBookLayout, WaitNoText);
    FULL_NERVE(PictureBookLayoutNrvFadeInText, PictureBookLayout, FadeInText);
    FULL_NERVE(PictureBookLayoutNrvWaitWithText, PictureBookLayout, WaitWithText);
    FULL_NERVE(PictureBookLayoutNrvFadeOutText, PictureBookLayout, FadeOutText);
    FULL_NERVE(PictureBookLayoutNrvPageNext, PictureBookLayout, PageNext);
    FULL_NERVE(PictureBookLayoutNrvFadeOut, PictureBookLayout, FadeOut);
    FULL_NERVE(PictureBookLayoutNrvClose, PictureBookLayout, Close);
};

namespace {
    void setTextOrDefault(LayoutActor* pBookLayout, const char* pPaneName, const char* pMessageLabel, const wchar_t* pFailsafeText) {
        const wchar_t* pMsg = MR::getGameMessageDirect(pMessageLabel);
        if (pMsg == nullptr)
            pMsg = pFailsafeText;

        MR::setTextBoxMessageRecursive(pBookLayout, pPaneName, pMsg);
    }
}

PictureBookLayout::PictureBookLayout(s32 chapterMin, s32 chapterMax, bool isRosettaReading) :
    LayoutActor("絵本レイアウト", true),
    mChapterMin(chapterMin),
    mChapterMax(chapterMax),
    mChapterRosettaMax(chapterMax),
    mChapterNo(1),
    mPageNo(0),
    mTextIndex(0),
    mNotReadedChapterNo(-1),
    mNotReadedPageNo(-1),
    mNotReadedTextIndex(-1),
    _44(nullptr),
    _48(nullptr),
    mTitleTexMap(nullptr),
    mCoverFrontTexMap(nullptr),
    mCoverBackTexMap(nullptr),
    mNextItemDir(1),
    mIsNextItemFast(false),
    mIconAButton(nullptr),
    mContentsButtonPaneController(nullptr),
    mCloseButton(nullptr)
{
    if (!isRosettaReading) {
        mContentsButtonPaneController = new ButtonPaneController*[getChapterMax()];
    }
}

void PictureBookLayout::init(const JMapInfoIter& rIter) {
    initLayoutManagerWithTextBoxBufferLength("PictureBook", 512, 1);
    MR::createAndAddPaneCtrl(this, "AButtonPosition", 1);
    MR::createAndAddPaneCtrl(this, "Text", 1);
    MR::connectToSceneLayout(this);
    initTexture();

    mIconAButton = MR::createAndSetupIconAButton(this, true, false);

    if (mContentsButtonPaneController != nullptr) {
        initContentsButton();

        mCloseButton = new PictureBookCloseButton(true);
        mCloseButton->initWithoutIter();
    }

    initNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeIn::sInstance);
}

void PictureBookLayout::appear() {
    mChapterNo = mChapterMin;
    mChapterMax = getChapterNumberMax();
    mPageNo = 0;
    mTextIndex = 0;
    mNotReadedChapterNo = -1;
    mNotReadedPageNo = -1;
    mNotReadedTextIndex = -1;
    mNextItemDir = 1;
    mIsNextItemFast = false;

    if (mContentsButtonPaneController == nullptr) {
        MR::hidePaneRecursive(this, "Contents");
    }
    else {
        hideContents();
    }

    LayoutActor::appear();

    if (mContentsButtonPaneController == nullptr) {
        setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeIn::sInstance);
    }
    else {
        setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvOpen::sInstance);
    }

    MR::requestMovementOn(this);
    MR::requestMovementOn(mIconAButton);
    MR::pauseOnCameraDirector();
    MR::deactivateGameSceneDraw3D();
}

void PictureBookLayout::kill() {
    LayoutActor::kill();
    mIconAButton->kill();

    if (mContentsButtonPaneController != nullptr) {
        mCloseButton->kill();
    }

    MR::startCurrentStageBGM(false);
    MR::pauseOffCameraDirector();
    MR::activateGameSceneDraw3D();
}

s32 PictureBookLayout::getChapterMax() {
    return 9;
}

void PictureBookLayout::control() {
    if (mContentsButtonPaneController == nullptr) {
        return;
    }

    for (s32 i = 0; i < getChapterMax(); i++) {
        mContentsButtonPaneController[i]->update();
    }
}

void PictureBookLayout::initTexture() {
    s32 textureNum = 0;

    for (s32 c = mChapterMin; c <= mChapterMax; c++) {
        textureNum += getTextureNum(c);
    }

    _44 = new nw4r::lyt::TexMap*[textureNum];

    s32 textureIndex = 0;

    for (s32 c = mChapterMin; c <= mChapterMax; c++) {
        char chapterArcName[64];
        snprintf(chapterArcName, sizeof(chapterArcName), "PictureBookChapter%d.arc", c);

        for (s32 p = 0; p < getTextureNum(c); p++) {
            char pageTexName[64];
            snprintf(pageTexName, sizeof(pageTexName), "Chapter%dPage%d.bti", c, p + 1);

            _44[textureIndex] = MR::createLytTexMap(chapterArcName, pageTexName);
            textureIndex++;
            // OSReport("%s\n", pageTexName);
        }
    }

    mTitleTexMap = MR::createLytTexMap("PictureBookTexture.arc", "PictureBookTitle.bti");
    mCoverFrontTexMap = MR::createLytTexMap("PictureBookTexture.arc", "PictureBookCoverFront.bti");
    mCoverBackTexMap = MR::createLytTexMap("PictureBookTexture.arc", "PictureBookCoverBack.bti");
}

void PictureBookLayout::initContentsButton() {
    char messageId[64];

    for (s32 i = 0; i < getChapterMax(); i++) {
        /* char contentsPaneName[64];
        char contentsPointingPaneName[64];
        snprintf(contentsPaneName, sizeof(contentsPaneName), "Chapter%d", i + 1);
        snprintf(contentsPointingPaneName, sizeof(contentsPointingPaneName), "BoxButton%d", i + 1); */
        mContentsButtonPaneController[i] = new ButtonPaneController(this, cContentsPaneName[i], cContentsPointingPaneName[i], 0, false);
        OSReport(cContentsPaneName[i]);
        OSReport("\n");
        // mContentsButtonPaneController[i]->invalidateAppearance();    Seems like nothing strange happens when this is not on, so...

        snprintf(messageId, sizeof(messageId), "PictureBookChapter%d_Title", i + 1);
        setTextOrDefault(this, cContentsPaneName[i], messageId, L"Untitled Chapter");
    }
}

bool PictureBookLayout::updateText() {
    char messageId[64];

    if (mPageNo == 0) {
        
        snprintf(messageId, sizeof(messageId), "PictureBookChapter%d_Title", mChapterNo);
        setTextOrDefault(this, "Title", messageId, L"- Untitled Chapter -");

        return true;
    }
    else {
        snprintf(messageId, sizeof(messageId), "PictureBookChapter%d_Page%d_%03d", mChapterNo, mPageNo, mTextIndex);

        if (MR::isExistGameMessage(messageId)) {
            setTextOrDefault(this, "Text", messageId, L"[...]");

            return true;
        }

        return false;
    }
}

void PictureBookLayout::updateTexture() {
    s32 textureNum = getTextureNum(mChapterNo);
    s32 pageNo;

    if (mNextItemDir > 0) {
        pageNo = mPageNo - 1;
    }
    else {
        pageNo = mPageNo;
    }

    if (pageNo == 0) {
        nw4r::lyt::TexMap* pTexMap = mTitleTexMap;

        MR::replacePaneTexture(this, "PicLeftPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnRightPage", pTexMap, 0);
    }
    else {
        s32 texMapIndex = textureNum + pageNo - 1;
        s32 ind = texMapIndex % textureNum + 1;
        nw4r::lyt::TexMap* pTexMap = _48[ind-1];

        MR::replacePaneTexture(this, "PicLeftPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnRightPage", pTexMap, 0);
    }

    if (mNextItemDir > 0) {
        pageNo = mPageNo;
    }
    else {
        pageNo = mPageNo + 1;
    }

    if (pageNo == 0) {
        nw4r::lyt::TexMap* pTexMap = mTitleTexMap;

        MR::replacePaneTexture(this, "PicRightPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnLeftPage", pTexMap, 0);
    }
    else {
        s32 texMapIndex = textureNum + pageNo - 1;
        s32 ind = texMapIndex % textureNum + 1;
        nw4r::lyt::TexMap* pTexMap = _48[ind-1];
        
        MR::replacePaneTexture(this, "PicRightPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnLeftPage", pTexMap, 0);
    }
}

bool PictureBookLayout::textNext() {
    if (mPageNo == 0) {
        return false;
    }

    mTextIndex += mNextItemDir;

    if (mTextIndex < 0) {
        return false;
    }

    return updateText();
}

bool PictureBookLayout::pageNext() {
    mPageNo += mNextItemDir;

    if (mPageNo < 0) {
        return false;
    }

    if (mPageNo != 0) {
        if (mNextItemDir > 0) {
            mTextIndex = 0;
        }
        else {
            mTextIndex = getCurrentMaxTextIndex();
        }
    }

    return updateText();
}

bool PictureBookLayout::chapterNext() {
    mChapterNo += mNextItemDir;

    if (mChapterMax < mChapterNo) {
        return false;
    }

    if (mNextItemDir > 0) {
        mPageNo = 0;
        mTextIndex = 0;
    }
    else {
        mPageNo = getPageNum(mChapterNo);
        mTextIndex = getCurrentMaxTextIndex();
    }

    return true;
}

void PictureBookLayout::updateTexMapChapterBase() {
    _48 = _44;

    for (s32 c = mChapterMin; c <= mChapterMax; c++) {
        if (c == mChapterNo) {
            break;
        }

        _48 = &_48[getTextureNum(c)];
    }
}

bool PictureBookLayout::isReadedCurrentText() const {
    bool r7;
    bool r5;

    if (mContentsButtonPaneController) {
        return true;
    }
    bool result = true;
    r7 = true;
    if (mChapterNo >= mNotReadedChapterNo) {
        r5 = false;
        if (mChapterNo == mNotReadedChapterNo) {
            if (mPageNo < mNotReadedPageNo) {
                r5 = true;
            }
        }
        if (!r5) {
            r7 = false;
        }
    }
    if (!r7) {
        r5 = false;
        if (mChapterNo == mNotReadedChapterNo) {
            if (mPageNo == mNotReadedPageNo) {
                if (mTextIndex <= mNotReadedTextIndex) {
                    r5 = true;
                }
            }
        }
        if (!r5) {
            result = false;
        }
    }
    return result;
}

s32 PictureBookLayout::getReadSpeed() const {
    bool b = mContentsButtonPaneController != nullptr || mIsNextItemFast;

    if (b) {
        return 3;
    }
    else {
        return 1;
    }
}

bool PictureBookLayout::isBookEndCurrentText() const {
    bool r31 = false;
    bool r30 = false;

    if (mChapterNo == 9) {
        if (mPageNo ==  getPageNum(mChapterNo)) {
            r30 = true;
        }
    }
    if (r30) {
        if (getCurrentMaxTextIndex() == mTextIndex) {
            r31 = true;
        }
    }
    return r31;
}

void PictureBookLayout::setTextAlpha(f32 alpha) {
    MR::setPaneAlphaFloat(this, "Text", alpha);
    MR::setPaneAlphaFloat(this, "Title", alpha);
    MR::setPaneAlphaFloat(this, "Contents", alpha);
}

s32 PictureBookLayout::getChapterNumberMax() const {
    if (mContentsButtonPaneController == nullptr) {
        return mChapterRosettaMax;
    }
    return 9; // return MR::getPictureBookChapterAlreadyRead();     GameFlag function, needs to be replaced
}

bool PictureBookLayout::isValidCloseButton() const {
    if (mContentsButtonPaneController == nullptr) {
        return false;
    }

    return isNerve(&NrvPictureBookLayout::PictureBookLayoutNrvOpen::sInstance)
        || isNerve(&NrvPictureBookLayout::PictureBookLayoutNrvContentsSelect::sInstance)
        || isNerve(&NrvPictureBookLayout::PictureBookLayoutNrvOpen::sInstance)
        || mPageNo == 0;
}

bool PictureBookLayout::isSelectedCloseButton() const {
    return isValidCloseButton() && mCloseButton->isSelected();
}

s32 PictureBookLayout::getCurrentMaxTextIndex() const {
    s32 chapterNo;
    s32 pageNo;

    pageNo = mPageNo;
    chapterNo = mChapterNo;

    for (s32 i = 0; i < 100; i++) {
        char messageId[64];
        snprintf(messageId, sizeof(messageId), "PictureBookChapter%d_Page%d_%03d", chapterNo, pageNo, i);

        if (MR::isExistGameMessage(messageId)) {
            continue;
        }

        return i - 1;
    }

    return 0;
}

void PictureBookLayout::exeOpen() {
    nw4r::lyt::TexMap* pTexMap;

    if (MR::isFirstStep(this)) {
        updateTexMapChapterBase();
        updateText();
        MR::hidePaneRecursive(this, "Text");
        MR::hidePaneRecursive(this, "Title");
        MR::hidePaneRecursive(this, "PicToneDown");
        pTexMap = mCoverFrontTexMap;
        MR::replacePaneTexture(this, "PicLeftPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnRightPage", pTexMap, 0);
        pTexMap = mTitleTexMap;
        MR::replacePaneTexture(this, "PicRightPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnLeftPage", pTexMap, 0);
        MR::startAnim(this, "Appear", 0);
        MR::setAnimFrameAndStop(this, 0.0f, 0);
        MR::openWipeFade(cFadeFrame);
        MR::startStageBGM("STM_PROLOGUE_01", false);
    }
    if (MR::isStep(this, cBookOpenFrame)) {
        MR::setAnimRate(this, 1.0f, 0);
    }

    if (MR::isStep(this, 140)) {
        MR::startSystemSE("SE_SY_PICTUREBOOK_NEXT_ST", -1, -1);
    }

    // s32 animFrameMax = MR::getAnimFrameMax(this, (u32)0);    // The game does not read this right and it just returns 0
    s32 animFrameMax = 160;  
    s32 stepMin = animFrameMax + cBookOpenFrame;
    s32 stepMax = stepMin + cFadeTextFrame;

    if (MR::isStep(this, stepMin)) {
        for (s32 i = 0; i < mChapterMax; i++) {
            mContentsButtonPaneController[i]->appear();
        }

        mCloseButton->appear();
    }
    
    if (MR::isGreaterEqualStep(this, stepMin)) {
        setTextAlpha(MR::calcNerveRate(this, stepMin, stepMax));

        if (isValidCloseButton()) {
            MR::requestStarPointerModePictureBook(this);
        }
    }
    MR::setNerveAtStep(this, &NrvPictureBookLayout::PictureBookLayoutNrvContentsSelect::sInstance, stepMax);
}

void PictureBookLayout::exeContentsSelect() {
    if (isValidCloseButton()) {
        MR::requestStarPointerModePictureBook(this);
    }
    if (mCloseButton->trySelect()) {
        setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvContentsFadeOut::sInstance);
    }
    else {
        for (s32 i = 0; i < mChapterMax; i++) {
            if (mContentsButtonPaneController[i]->isPointingTrigger()) {
                MR::startSystemSE("SE_SY_PICBOOK_CONTENTS_CUR", -1, -1);
            }

            if (mContentsButtonPaneController[i]->trySelect()) {
                MR::startSystemSE("SE_SY_TALK_OK", -1, -1);
                mChapterNo = i + 1;
                mCloseButton->disappear();
                setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvContentsFadeOut::sInstance);
                break;
            }
        }
    }
}

void PictureBookLayout::exeContentsFadeOut() {
    s32 stepMin = 20; // MR::getPaneAnimFrameMax(this, cContentsPaneName[mChapterNo - 1], 0);
    s32 stepMax = stepMin + cFadeTextFrame;

    if (MR::isGreaterEqualStep(this, stepMin)) {
        setTextAlpha(1.0f - MR::calcNerveRate(this, stepMin, stepMax));
    }

    if (MR::isLessStep(this, stepMax)) {
        if (isValidCloseButton()) {
            MR::requestStarPointerModePictureBook(this);
        }
    }

    if (isSelectedCloseButton()) {
        if (MR::isGreaterStep(this, stepMax)) {
            if (MR::isDead(mCloseButton)) {
                hideContents();
                setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvClose::sInstance);
            }
        }
    } else {
        if (MR::isStep(this, stepMax)) {
            MR::closeWipeFade(cFadeFrame);
        }

        if (MR::isGreaterStep(this, stepMax) && !MR::isWipeActive()) {
            hideContents();
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeIn::sInstance);
        }
    }
}

void PictureBookLayout::exeFadeIn() {
    if (MR::isFirstStep(this)) {
        updateTexMapChapterBase();
        updateText();
        MR::hidePaneRecursive(this, "Text");
        MR::hidePaneRecursive(this, "Title");
        MR::hidePaneRecursive(this, "PicToneDown");
        MR::showPaneRecursive(this, "PicToneDown");

        f32 alpha;

        if (mNextItemDir > 0) {
            alpha = 0.0f;
        }
        else {
            alpha = 1.0f;
        }

        MR::setPaneAlphaFloat(this, "PicToneDown", alpha);

        mNextItemDir = -1; // I have no clue of why is this needed now

        updateTexture();
        
        MR::startAnim(this, "PageNext", 0);

        f32 animFrame;

        if (mNextItemDir > 0) {
            animFrame = 60; // = MR::getAnimFrameMax(this, (u32)0);
        }
        else {
            animFrame = 0.0f;
        }

        MR::setAnimFrameAndStop(this, animFrame, 0);
        MR::openWipeFade(cFadeFrame / getReadSpeed());

        if (mContentsButtonPaneController == nullptr) {
            MR::startStageBGM("STM_PROLOGUE_01", false);
        }
    }

    if (!MR::isWipeActive()) {
        setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvWaitNoText::sInstance);
    }
}

void PictureBookLayout::exeWaitNoText() {
    bool b = mContentsButtonPaneController != nullptr || mIsNextItemFast;

    MR::setNerveAtStep(this, &NrvPictureBookLayout::PictureBookLayoutNrvFadeInText::sInstance, b ? 0 : cWaitNoTextFrame);
}

void PictureBookLayout::exeFadeInText() {
    if (MR::isFirstStep(this)) {
        if (mPageNo == 0) {
            MR::showPaneRecursive(this, "Title");

            if (mContentsButtonPaneController != nullptr) {
                mCloseButton->appear();
            }
        }
        else {
            MR::showPaneRecursive(this, "Text");
            MR::showPaneRecursive(this, "PicToneDown");
            MR::startPaneAnim(this, "Text", "TextColor", 0);

            if (isReadedCurrentText()) {
                MR::setPaneAnimFrameAndStop(this, "Text", 1.0f, 0);
            }
            else {
                MR::setPaneAnimFrameAndStop(this, "Text", 0.0f, 0);
            }
        }

        // Here there was a bunch of code for the music on special pages //
       
        if (isBookEndCurrentText()) {
            setTextBoxHorizontalPositionRecursive(this, "Text", 1);
            MR::setTextBoxVerticalPositionCenterRecursive(this, "Text");
        }
        else {
            setTextBoxHorizontalPositionRecursive(this, "Text", 0);
            setTextBoxVerticalPositionRecursive(this, "Text", 0);   // Crazy that TopRecursive isn't in SMG2 when it would be just a copy of SMG1
        }

        if (mIsNextItemFast && !isReadedCurrentText()) {
            mIsNextItemFast = false;
        }
    }

    if (isValidCloseButton()) {
        MR::requestStarPointerModePictureBook(this);
    }

    s32 step = 30 / getReadSpeed();
    f32 alpha = MR::calcNerveRate(this, step);

    MR::setPaneAlphaFloat(this, "Text", alpha);
    MR::setPaneAlphaFloat(this, "Title", alpha);
    MR::setPaneAlphaFloat(this, "Contents", alpha);
    MR::setPaneAlphaFloat(this, "PicToneDown", getFadeInAlphaTextBG(alpha));
    MR::setNerveAtStep(this, &NrvPictureBookLayout::PictureBookLayoutNrvWaitWithText::sInstance, step);
}

void PictureBookLayout::exeWaitWithText() {
    if (MR::isFirstStep(this)) {
        openWithTurn(mIconAButton);
    }

    if (isValidCloseButton()) {
        MR::requestStarPointerModePictureBook(this);
    }

    if (isValidCloseButton() && mCloseButton->trySelect()) {
        setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeOutText::sInstance);
    }
    else {
        bool isTriggerNextPage = MR::testCorePadTriggerA(0)
            || MR::testCorePadTriggerRight(0)
            || MR::testSubPadStickTriggerRight(0);

        if (isTriggerNextPage) {
            MR::startSystemSE("SE_SY_TALK_FOCUS_ITEM", -1, -1);

            if (mNextItemDir > 0 && !isReadedCurrentText()) {
                mNotReadedChapterNo = mChapterNo;
                mNotReadedPageNo = mPageNo;
                mNotReadedTextIndex = mTextIndex;
            }

            if (isValidCloseButton()) {
                mCloseButton->disappear();
            }

            mNextItemDir = 1;

            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeOutText::sInstance);
        }
        else {
            bool isTriggerPrevPage = MR::testCorePadTriggerLeft(0)
                || MR::testSubPadStickTriggerLeft(0);

            if (isTriggerPrevPage) {
                if (mChapterMin < mChapterNo || mPageNo > 0 || mTextIndex > 0) {
                    MR::startSystemSE("SE_SY_TALK_FOCUS_ITEM", -1, -1);

                    if (isValidCloseButton()) {
                        mCloseButton->disappear();
                    }

                    mIsNextItemFast = true;
                    mNextItemDir = -1;

                    setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeOutText::sInstance);
                }
            }
        }
    }
}

void PictureBookLayout::exeFadeOutText() {
    if (MR::isFirstStep(this)) {
        mIconAButton->term();
    }

    if (isValidCloseButton()) {
        MR::requestStarPointerModePictureBook(this);
    }

    s32 step = 30 / getReadSpeed();
    f32 alpha = 1.0f - MR::calcNerveRate(this, step);

    MR::setPaneAlphaFloat(this, "Text", alpha);
    MR::setPaneAlphaFloat(this, "Title", alpha);
    MR::setPaneAlphaFloat(this, "Contents", alpha);
    MR::setPaneAlphaFloat(this, "PicToneDown", getFadeOutAlphaTextBG(alpha));

    if (isSelectedCloseButton()) {
        if (MR::isGreaterStep(this, step) && MR::isDead(mCloseButton)) {
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvClose::sInstance);
        }
    }
    else if (MR::isStep(this, step)) {
        if (textNext()) {
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeInText::sInstance);
        }
        else if (pageNext()) {
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvPageNext::sInstance);
        }
        else if (chapterNext()) {
            if (MR::isPlayingStageBgmName("STM_PROLOGUE_01_B")) {
                MR::stopStageBGM(120);
            }
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeOut::sInstance);
        }
        else if (mContentsButtonPaneController == nullptr) {
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeOut::sInstance);
        }
        else {
            setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvClose::sInstance);
        }
    }
}

void PictureBookLayout::exePageNext() {
    if (MR::isFirstStep(this)) {
        MR::hidePaneRecursive(this, "Text");
        MR::hidePaneRecursive(this, "Title");
        MR::hidePaneRecursive(this, "PicToneDown");
        updateTexture();

        if (mNextItemDir > 0) {
            MR::startAnim(this, "PageNext", 0);
        }
        else {
            MR::startAnimReverseOneTime(this, "PageNext", 0);
        }

        if (mIsNextItemFast) {
            MR::startSystemSE("SE_SY_PICTUREBOOK_NEXT_F_ST", -1, -1);
        }
        else {
            MR::startSystemSE("SE_SY_PICTUREBOOK_NEXT_ST", -1, -1);
        }

        MR::setAnimRate(this, mNextItemDir * getReadSpeed(), 0);
    }

    if (mIsNextItemFast) {
        if (MR::isStep(this, cPageNextEndFastSeStep)) {
            MR::startSystemSE("SE_SY_PICTUREBOOK_NEXT_F_ED", -1, -1);
        }
    }
    else if (MR::isStep(this, cPageNextEndNormalSeStep)) {
        MR::startSystemSE("SE_SY_PICTUREBOOK_NEXT_ED", -1, -1);
    }

    MR::setNerveAtAnimStopped(this, &NrvPictureBookLayout::PictureBookLayoutNrvWaitNoText::sInstance, 0);
}

void PictureBookLayout::exeFadeOut() {
    if (MR::isFirstStep(this)) {
        MR::closeWipeFade(cFadeFrame / getReadSpeed());

        if (mContentsButtonPaneController == nullptr) {
            MR::stopStageBGM(cFadeFrame / getReadSpeed());
        }
    }

    if (MR::isWipeActive()) {
        return;
    }

    if (mChapterMax >= mChapterNo) {
        setNerve(&NrvPictureBookLayout::PictureBookLayoutNrvFadeIn::sInstance);
    }
    else {
        kill();
    }
}

void PictureBookLayout::exeClose() {
    nw4r::lyt::TexMap* pTexMap;

    if (MR::isFirstStep(this)) {
        MR::hidePaneRecursive(this, "Text");
        MR::hidePaneRecursive(this, "Title");
        MR::hidePaneRecursive(this, "PicToneDown");
        MR::startAnim(this, "End", 0);

        if (isSelectedCloseButton()) {
            pTexMap = mTitleTexMap;

            MR::replacePaneTexture(this, "PicLeftPage", pTexMap, 0);
            MR::replacePaneTexture(this, "PicTurnRightPage", pTexMap, 0);
        } else {
            pTexMap = _48[mPageNo - 2];

            MR::replacePaneTexture(this, "PicLeftPage", pTexMap, 0);
            MR::replacePaneTexture(this, "PicTurnRightPage", pTexMap, 0);
        }

        pTexMap = mCoverBackTexMap;

        MR::replacePaneTexture(this, "PicRightPage", pTexMap, 0);
        MR::replacePaneTexture(this, "PicTurnLeftPage", pTexMap, 0);
    }

    // s32 step = MR::getAnimFrameMax(this, (u32)0);    // Okay this thing doesn't work. Maybe because it's a SMG1 layout?
    s32 step = 160;

    if (MR::isStep(this, step)) {
        MR::closeWipeFade(cFadeFrame);
        MR::stopStageBGM(cFadeFrame);
    }

    if (MR::isStep(this, 150)) {
        MR::startSystemSE("SE_SY_PICTUREBOOK_END", -1, -1);
    }

    if (MR::isGreaterStep(this, step)) {
        if (!MR::isWipeActive()) {
            kill();
        }
    }
}

void PictureBookLayout::hideContents() {
    for (s32 i = 0; i < getChapterMax(); i++) {
        mContentsButtonPaneController[i]->forceToHide();
    }
}

f32 PictureBookLayout::getFadeInAlphaTextBG(f32 alpha) const {
    bool var;
    if (!mPageNo || isBookEndCurrentText()) {
        return 0.0f;
    }
    var = false;
    if (mNextItemDir > 0) {
        if (!mTextIndex) {
            var = true;
        }
    }
    else if (getCurrentMaxTextIndex() == mTextIndex) {
        if (mPageNo < getTextureNum(mChapterNo)) {
            var = true;
        }
    }
    if (var) {
        return alpha;
    }
    return 1.0f;
}

f32 PictureBookLayout::getFadeOutAlphaTextBG(f32 alpha) const {
    bool var;
    if (!mPageNo || isBookEndCurrentText()) {
        return 0.0f;
    }
    var = false;
    if (mNextItemDir > 0) {
         if (getCurrentMaxTextIndex() == mTextIndex) {
            if (mPageNo < getTextureNum(mChapterNo)) {
                var = true;
            }
        }
    }
    else if(mTextIndex == 0 && mPageNo > 0){
        var = true;
    }
    if (var) {
        return alpha;
    }
    return 1.0f;
}