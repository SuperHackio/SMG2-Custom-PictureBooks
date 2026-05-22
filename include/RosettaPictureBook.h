#pragma once

#include "syati.h"

class IconAButton;
class PictureBookLayout;

class RosettaPictureBook : public LiveActor {
public:
    /// @brief Creates a new `RosettaPictureBook`.
    /// @param pName A pointer to the null-terminated name of the object.
    RosettaPictureBook(const char* pName);

    virtual void init(const JMapInfoIter& rIter);
    virtual void appear();
    virtual void kill();
    virtual void control();
    virtual void attackSensor(HitSensor* pSender, HitSensor* pReceiver);

    void exeWait();
    void exeDemoWait();
    void exeFadeOut();
    void exeReading();
    void exeFadeIn();

private:
    /* 0x8C */ PictureBookLayout* mLayout;
    /* 0x90 */ IconAButton* mIconAButton;
    /* 0x94 */ bool mIsValidOpenIconAButton;
};