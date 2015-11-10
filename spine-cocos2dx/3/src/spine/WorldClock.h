#ifndef SPINE_WORLD_CLOCK_H
#define SPINE_WORLD_CLOCK_H

#include <spine/MySkeletonAnimation.h>

namespace spine {

class WorldClock
{
private:
    static WorldClock *instance;

    bool _dirty;
    bool _isPlaying;
    float _time;
    float _timeScale;



    std::vector<MySkeletonAnimation*> _animatableList;

public:
    static WorldClock* getInstance();

    float getTime() const;

    float getTimeScale() const;
    void setTimeScale(float timeScale);

public:
    WorldClock(float time = -1, float timeScale = 1);
    virtual ~WorldClock();

    virtual bool contains(const MySkeletonAnimation *animatable) const;
    virtual void add(MySkeletonAnimation *animatable);
    virtual void remove(MySkeletonAnimation *animatable);
    virtual void removeAll();
    virtual void play();
    virtual void stop();
    virtual void advanceTime(float passedTime);

};

}
#endif  // SPINE_WORLD_CLOCK_H