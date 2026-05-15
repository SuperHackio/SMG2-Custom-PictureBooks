#include "RosettaPictureBook.h"
#include "PictureBookLayout.h"

namespace MR {
    s32 getPictureBookChapterCanRead() {
        return 9;   // Possible BCSV setup later
    }
}

namespace {
    const s32 hFadeOutFrame = 60;
    const s32 hFadeInFrame = 60;
};

namespace NrvRosettaPictureBook {
    FULL_NERVE(HostTypeNrvWait, RosettaPictureBook, Wait);
    FULL_NERVE(HostTypeNrvDemoWait, RosettaPictureBook, DemoWait);
    FULL_NERVE(HostTypeNrvFadeOut, RosettaPictureBook, FadeOut);
    FULL_NERVE(HostTypeNrvReading, RosettaPictureBook, Reading);
    FULL_NERVE(HostTypeNrvFadeIn, RosettaPictureBook, FadeIn);
};

RosettaPictureBook::RosettaPictureBook(const char* pName)
    : LiveActor(pName), mLayout(nullptr), mIconAButton(nullptr), mIsValidOpenIconAButton(false) {}

void RosettaPictureBook::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm("RosettaPictureBook", nullptr, nullptr, false);
    MR::connectToSceneMapObj(this);
    initHitSensor(1);
    MR::addHitSensorMapObjSimple(this, "body", 8, 150.0f, TVec3f(0.0f, 0.0f, 0.0f));

    mLayout = new PictureBookLayout(1, MR::getPictureBookChapterCanRead(), false);
    mLayout->initWithoutIter();

    mIconAButton = new IconAButton(true, false);
    mIconAButton->initWithoutIter();

    initNerve(&NrvRosettaPictureBook::HostTypeNrvWait::sInstance, 0);
    MR::tryRegisterDemoCast(this, rIter);
    makeActorAppeared();
}

void RosettaPictureBook::appear() {
    mIsValidOpenIconAButton = false;

    setNerve(&NrvRosettaPictureBook::HostTypeNrvWait::sInstance);
    LiveActor::appear();
}

void RosettaPictureBook::kill() {
    LiveActor::kill();
    mIconAButton->kill();
}

void RosettaPictureBook::makeArchiveList(NameObjArchiveListCollector* pCollector, const JMapInfoIter& rIter) {
    PictureBookLayout::makeArchiveList(pCollector, 1, MR::getPictureBookChapterCanRead(), false);
}

void RosettaPictureBook::control() {
    mIsValidOpenIconAButton = false;
}

void RosettaPictureBook::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isOnGroundPlayer()) {
        mIsValidOpenIconAButton = true;
    }
}

void RosettaPictureBook::exeWait() {
    if (mIsValidOpenIconAButton) {
        if (!mIconAButton->isOpen()) {
            MR::startSystemSE("SE_SY_TALK_BUTTON_APPEAR", -1, -1);
            mIconAButton->openWithRead();
        }
    } else if (mIconAButton->isOpen()) {
        mIconAButton->term();
    }

    if (!mIsValidOpenIconAButton) {
        return;
    }

    if (!mIconAButton->isOpen()) {
        return;
    }

    if (!MR::testCorePadTriggerA(0)) {
        return;
    }
    
    DemoStartRequestUtil::requestStartDemo(  // Not 100% equal but...    MR::requestStartDemoMarioPuppetableWithoutCinemaFrame(LiveActor*, const char*, const Nerve*, const Nerve*)
        this,
        "ロゼッタ絵本デモ",
        &NrvRosettaPictureBook::HostTypeNrvFadeOut::sInstance,
        &NrvRosettaPictureBook::HostTypeNrvDemoWait::sInstance,
        2,
        DemoStartInfo::DEMOTYPE_0,
        DemoStartInfo::CINEMAFRAMETYPE_1,
        DemoStartInfo::STARPOINTERTYPE_0,
        DemoStartInfo::DELETEEFFECTYPE_0);
}

void RosettaPictureBook::exeDemoWait() {}

void RosettaPictureBook::exeFadeOut() {
    if (MR::isFirstStep(this)) {
        MR::startSystemSE("SE_SY_TALK_START", -1, -1);
        MR::requestMovementOn(mIconAButton);
        mIconAButton->term();
        MR::closeWipeCircle(hFadeOutFrame);
        MR::stopStageBGM(hFadeOutFrame);
        MR::startBckPlayer("Wait", (const char*)nullptr);
    }

    if (MR::isGreaterStep(this, hFadeOutFrame)) {
        mLayout->appear();
        setNerve(&NrvRosettaPictureBook::HostTypeNrvReading::sInstance);
    }
}

void RosettaPictureBook::exeReading() {
    if (MR::isFirstStep(this)) {
        mLayout->appear();
    }

    if (MR::isDead(mLayout)) {
        MR::endDemo(this, "ロゼッタ絵本デモ");
        setNerve(&NrvRosettaPictureBook::HostTypeNrvFadeIn::sInstance);
    }
}

void RosettaPictureBook::exeFadeIn() {
    if (MR::isFirstStep(this)) {
        MR::openWipeCircle(hFadeInFrame);
    }

    if (MR::isGreaterStep(this, hFadeInFrame)) {
        setNerve(&NrvRosettaPictureBook::HostTypeNrvWait::sInstance);
    }
}

