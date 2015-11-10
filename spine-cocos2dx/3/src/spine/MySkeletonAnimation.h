/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.1
 * 
 * Copyright (c) 2013, Esoteric Software
 * All rights reserved.
 * 
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to install, execute and perform the Spine Runtimes
 * Software (the "Software") solely for internal use. Without the written
 * permission of Esoteric Software (typically granted by licensing Spine), you
 * may not (a) modify, translate, adapt or otherwise create derivative works,
 * improvements of the Software or develop new applications using the Software
 * or (b) remove, delete, alter or obscure any trademarks or any copyright,
 * trademark, patent or other intellectual property or proprietary rights
 * notices on or in the Software, including any copy thereof. Redistributions
 * in binary or source form must include this license and terms.
 * 
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef SPINE_MYSKELETONANIMATION_H_
#define SPINE_MYSKELETONANIMATION_H_

#include <spine/spine.h>
#include <spine/SkeletonAnimation.h>
#include "cocos2d.h"

namespace spine {

class MySkeletonAnimation: public SkeletonAnimation {

public:
//     static MySkeletonAnimation* createWithData (spSkeletonData* skeletonData);
//     static MySkeletonAnimation* createWithFile (const std::string& skeletonDataFile, spAtlas* atlas, float scale = 1);
    static MySkeletonAnimation* createWithFile (const std::string& skeletonDataFile, const std::string& atlasFile, float scale = 1);

	virtual void update (float deltaTime) override;
	virtual void advanceTime(float dt);
    virtual void setAdvanceTimeByClock(bool isClock);
    virtual bool isAdvaceTimeByClock() const;

    virtual void pauseAnimation();
    virtual void resumeAnimation();
    virtual bool isRunningAnimation();

    virtual void draw (cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t transformFlags) override;
    virtual void splitDraw (cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t transformFlags);
    virtual void drawSkeleton (const cocos2d::Mat4& transform, uint32_t transformFlags) override;
    virtual void drawSplitSlot(const cocos2d::Mat4& transform, uint32_t transformFlags, const int begin, const int end);
    virtual void drawDebug (const cocos2d::Mat4& transform, uint32_t transformFlags);
    virtual bool hasBone(const std::string& boneName) const;
    virtual bool hasAnimation(const std::string& animationName) const;
    virtual cocos2d::Vec2 getBoneWorldPos(const std::string& boneName) const;
    virtual cocos2d::Rect getInnerBoundingBox() const;

    virtual cocos2d::Node* getSlotNode(const std::string& slotName);
    virtual cocos2d::Node* bindSlotNode(const std::string& slotName, cocos2d::Node *node = nullptr);
    virtual void unbindSlotNode(const std::string& slotName);

protected:
    MySkeletonAnimation (spSkeletonData* skeletonData);
    MySkeletonAnimation (const std::string&skeletonDataFile, spAtlas* atlas, float scale = 1);
    MySkeletonAnimation (const std::string& skeletonDataFile, const std::string& atlasFile, float scale = 1);
    ~MySkeletonAnimation();

protected:
    typedef std::map<const spSlot*, cocos2d::Node*> SlotNodeMap;
    typedef std::map<const cocos2d::Node*, spSlot*> NodeSlotMap;
  
    typedef SlotNodeMap::iterator SlotNodeIter;  
    SlotNodeMap _slotNodeMap;
    NodeSlotMap _nodeSlotMap;

private:
    bool _isRunningAnimation;
    std::vector<cocos2d::CustomCommand*> _splitDrawCommands;
};

}

#endif /* SPINE_MYSKELETONANIMATION_H_ */