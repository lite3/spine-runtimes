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

#include <spine/MySkeletonAnimation.h>
#include <spine/WorldClock.h>
#include <spine/spine-cocos2dx.h>
#include <spine/extension.h>
#include <spine/PolygonBatch.h>
#include <algorithm>

USING_NS_CC;
using std::min;
using std::max;

namespace spine {
//

static const int quadTriangles[6] = {0, 1, 2, 2, 3, 0};

// MySkeletonAnimation* MySkeletonAnimation::createWithData (spSkeletonData* skeletonData) {
//     MySkeletonAnimation* node = new MySkeletonAnimation(skeletonData);
//     node->autorelease();
//     return node;
// }
// 
// MySkeletonAnimation* MySkeletonAnimation::createWithFile (const std::string& skeletonDataFile, spAtlas* atlas, float scale) {
//     MySkeletonAnimation* node = new MySkeletonAnimation(skeletonDataFile, atlas, scale);
//     node->autorelease();
//     return node;
// }

MySkeletonAnimation* MySkeletonAnimation::createWithFile (const std::string& skeletonDataFile, const std::string& atlasFile, float scale) {
    MySkeletonAnimation* node = new MySkeletonAnimation(skeletonDataFile, atlasFile, scale);
    node->autorelease();
    return node;
}

MySkeletonAnimation::MySkeletonAnimation (spSkeletonData *skeletonData)
		: SkeletonAnimation(skeletonData)
        , _isRunningAnimation(true)
        , _splitDrawCommands()
{
}

MySkeletonAnimation::MySkeletonAnimation (const std::string& skeletonDataFile, spAtlas* atlas, float scale)
		: SkeletonAnimation(skeletonDataFile, atlas, scale)
        , _isRunningAnimation(true)
        , _splitDrawCommands()
{
}

MySkeletonAnimation::MySkeletonAnimation (const std::string& skeletonDataFile, const std::string& atlasFile, float scale)
		: SkeletonAnimation(skeletonDataFile, atlasFile, scale)
        , _isRunningAnimation(true)
        , _splitDrawCommands()
{
}

MySkeletonAnimation::~MySkeletonAnimation () {
	if(isAdvaceTimeByClock()) {
        WorldClock::getInstance()->remove(this);
    } else {
        unscheduleUpdate();
    }
    _slotNodeMap.clear();
    _nodeSlotMap.clear();
    for (auto command : _splitDrawCommands) {
        delete command;
    }
    _splitDrawCommands.clear();
}

void MySkeletonAnimation::update (float deltaTime) {
    if (!_isRunningAnimation) return;

    retain();
    SkeletonAnimation::update(deltaTime);
    release();
}

void MySkeletonAnimation::advanceTime (float deltaTime) {
    if (!_isRunningAnimation) return;

    retain();
    SkeletonAnimation::update(deltaTime);
    release();
}

void MySkeletonAnimation::setAdvanceTimeByClock(bool isClock) {
    auto clock = WorldClock::getInstance();
    if (isClock == clock->contains(this)) return;
        
    if (isClock)
    {
        clock->add(this);
        unscheduleUpdate();
    }else
    {
        clock->remove(this);
        scheduleUpdate();
    }
}

bool MySkeletonAnimation::isAdvaceTimeByClock() const {
    return WorldClock::getInstance()->contains(this);
}

void MySkeletonAnimation::pauseAnimation() {
    _isRunningAnimation = false;
}
void MySkeletonAnimation::resumeAnimation() {
    _isRunningAnimation = true;
}
bool MySkeletonAnimation::isRunningAnimation() {
    return _isRunningAnimation;
}

void MySkeletonAnimation::draw (cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t transformFlags) {
    if (_slotNodeMap.size() > 0) {
        splitDraw(renderer, transform, transformFlags);
    } else {
        _drawCommand.init(_globalZOrder);
        _drawCommand.func = CC_CALLBACK_0(MySkeletonAnimation::drawSkeleton, this, transform, transformFlags);
        renderer->addCommand(&_drawCommand);
    }
}

void MySkeletonAnimation::splitDraw (cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t transformFlags) {
    auto commandSize = _splitDrawCommands.size();
    int useIdx = 0;
    int slotIdx = 0;

    for (int i = 0, n = _skeleton->slotsCount; i < n; i++) {
        spSlot* slot = _skeleton->drawOrder[i];
        if (!slot->attachment && i != n - 1) continue;

        auto iter = _slotNodeMap.find(slot);  // the end must be draw
        if (iter == _slotNodeMap.end() && i != n - 1) continue;

        CustomCommand *drawCommand = nullptr;
        if (useIdx < commandSize) {
            drawCommand = _splitDrawCommands.at(useIdx);
            useIdx++;
        } else {
            drawCommand = new CustomCommand();
            _splitDrawCommands.push_back(drawCommand);
        }
        drawCommand->init(_globalZOrder);
        drawCommand->func = CC_CALLBACK_0(MySkeletonAnimation::drawSplitSlot, this, transform, transformFlags, slotIdx, i+1);
        renderer->addCommand(drawCommand);
        slotIdx = i + 1;

        if (iter != _slotNodeMap.end()) {
            // update slot node STR
            auto node = iter->second;
            auto bone = slot->bone;
            node->setPosition(bone->worldX, bone->worldY);
            node->setRotation(-bone->worldRotation);
            node->setScale(bone->worldScaleX, bone->worldScaleY);
            node->setOpacity(255 * slot->a);
            node->setColor(ccc3(255*slot->r, 255*slot->g, 255*slot->b));

            // add node to renderer
            node->setVisible(true);
            node->visit(renderer, transform, transformFlags);
            node->setVisible(false);
            // if (!slot->attachment) continue;
        }
    }

    if (_debugBones || _debugBones) {
        _drawCommand.init(_globalZOrder);
        _drawCommand.func = CC_CALLBACK_0(MySkeletonAnimation::drawDebug, this, transform, transformFlags);
        renderer->addCommand(&_drawCommand);
    }
}


void MySkeletonAnimation::drawSkeleton (const cocos2d::Mat4& transform, uint32_t transformFlags) {
    getGLProgramState()->apply(transform);

    Color3B nodeColor = getColor();
    _skeleton->r = nodeColor.r / (float)255;
    _skeleton->g = nodeColor.g / (float)255;
    _skeleton->b = nodeColor.b / (float)255;
    _skeleton->a = getDisplayedOpacity() / (float)255;

    int additive = -1;
    Color4B color;
    const float* uvs = nullptr;
    int verticesCount = 0;
    const int* triangles = nullptr;
    int trianglesCount = 0;
    float r = 0, g = 0, b = 0, a = 0;
    for (int i = 0, n = _skeleton->slotsCount; i < n; i++) {
        spSlot* slot = _skeleton->drawOrder[i];
        if (!slot->attachment) continue;
        Texture2D *texture = nullptr;
        switch (slot->attachment->type) {
        case SP_ATTACHMENT_REGION: {
            spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
            spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
            texture = getTexture(attachment);
            uvs = attachment->uvs;
            verticesCount = 8;
            triangles = quadTriangles;
            trianglesCount = 6;
            r = attachment->r;
            g = attachment->g;
            b = attachment->b;
            a = attachment->a;
            break;
                                   }
        case SP_ATTACHMENT_MESH: {
            spMeshAttachment* attachment = (spMeshAttachment*)slot->attachment;
            spMeshAttachment_computeWorldVertices(attachment, slot, _worldVertices);
            texture = getTexture(attachment);
            uvs = attachment->uvs;
            verticesCount = attachment->verticesCount;
            triangles = attachment->triangles;
            trianglesCount = attachment->trianglesCount;
            r = attachment->r;
            g = attachment->g;
            b = attachment->b;
            a = attachment->a;
            break;
                                 }
        case SP_ATTACHMENT_SKINNED_MESH: {
            spSkinnedMeshAttachment* attachment = (spSkinnedMeshAttachment*)slot->attachment;
            spSkinnedMeshAttachment_computeWorldVertices(attachment, slot, _worldVertices);
            texture = getTexture(attachment);
            uvs = attachment->uvs;
            verticesCount = attachment->uvsCount;
            triangles = attachment->triangles;
            trianglesCount = attachment->trianglesCount;
            r = attachment->r;
            g = attachment->g;
            b = attachment->b;
            a = attachment->a;
            break;
                                         }
        default: ;
        } 
        if (texture) {
            if (slot->data->additiveBlending != additive) {
                _batch->flush();
                GL::blendFunc(_blendFunc.src, slot->data->additiveBlending ? GL_ONE : _blendFunc.dst);
                additive = slot->data->additiveBlending;
            }
            color.a = _skeleton->a * slot->a * a * 255;
            float multiplier = _premultipliedAlpha ? color.a : 255;
            color.r = _skeleton->r * slot->r * r * multiplier;
            color.g = _skeleton->g * slot->g * g * multiplier;
            color.b = _skeleton->b * slot->b * b * multiplier;
            _batch->add(texture, _worldVertices, uvs, verticesCount, triangles, trianglesCount, &color);
        }
    }
    _batch->flush();

    if (_debugSlots || _debugBones) {
        Director* director = Director::getInstance();
        director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
        director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, transform);

        if (_debugSlots) {
            // Slots.
            DrawPrimitives::setDrawColor4B(0, 0, 255, 255);
            glLineWidth(1);
            Vec2 points[4];
            V3F_C4B_T2F_Quad quad;
            for (int i = 0, n = _skeleton->slotsCount; i < n; i++) {
                spSlot* slot = _skeleton->drawOrder[i];
                if (!slot->attachment || slot->attachment->type != SP_ATTACHMENT_REGION) continue;
                spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
                spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
                points[0] = Vec2(_worldVertices[0], _worldVertices[1]);
                points[1] = Vec2(_worldVertices[2], _worldVertices[3]);
                points[2] = Vec2(_worldVertices[4], _worldVertices[5]);
                points[3] = Vec2(_worldVertices[6], _worldVertices[7]);
                DrawPrimitives::drawPoly(points, 4, true);
            }
        }
        if (_debugBones) {
            // Bone lengths.
            glLineWidth(2);
            DrawPrimitives::setDrawColor4B(255, 0, 0, 255);
            for (int i = 0, n = _skeleton->bonesCount; i < n; i++) {
                spBone *bone = _skeleton->bones[i];
                float x = bone->data->length * bone->m00 + bone->worldX;
                float y = bone->data->length * bone->m10 + bone->worldY;
                DrawPrimitives::drawLine(Vec2(bone->worldX, bone->worldY), Vec2(x, y));
            }
            // Bone origins.
            DrawPrimitives::setPointSize(4);
            DrawPrimitives::setDrawColor4B(0, 0, 255, 255); // Root bone is blue.
            for (int i = 0, n = _skeleton->bonesCount; i < n; i++) {
                spBone *bone = _skeleton->bones[i];
                DrawPrimitives::drawPoint(Vec2(bone->worldX, bone->worldY));
                if (i == 0) DrawPrimitives::setDrawColor4B(0, 255, 0, 255);
            }
        }
        director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    }
}

void MySkeletonAnimation::drawSplitSlot(const cocos2d::Mat4& transform, uint32_t transformFlags, const int begin, const int end) {
    getGLProgramState()->apply(transform);

    Color3B nodeColor = getColor();
    _skeleton->r = nodeColor.r / (float)255;
    _skeleton->g = nodeColor.g / (float)255;
    _skeleton->b = nodeColor.b / (float)255;
    _skeleton->a = getDisplayedOpacity() / (float)255;

    int additive = -1;
    Color4B color;
    const float* uvs = nullptr;
    int verticesCount = 0;
    const int* triangles = nullptr;
    int trianglesCount = 0;
    float r = 0, g = 0, b = 0, a = 0;
    for (int i = begin; i < end; i++) {
        spSlot* slot = _skeleton->drawOrder[i];
        if (!slot->attachment) continue;
        Texture2D *texture = nullptr;
        switch (slot->attachment->type) {
        case SP_ATTACHMENT_REGION: {
            spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
            spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
            texture = getTexture(attachment);
            uvs = attachment->uvs;
            verticesCount = 8;
            triangles = quadTriangles;
            trianglesCount = 6;
            r = attachment->r;
            g = attachment->g;
            b = attachment->b;
            a = attachment->a;
            break;
                                   }
        case SP_ATTACHMENT_MESH: {
            spMeshAttachment* attachment = (spMeshAttachment*)slot->attachment;
            spMeshAttachment_computeWorldVertices(attachment, slot, _worldVertices);
            texture = getTexture(attachment);
            uvs = attachment->uvs;
            verticesCount = attachment->verticesCount;
            triangles = attachment->triangles;
            trianglesCount = attachment->trianglesCount;
            r = attachment->r;
            g = attachment->g;
            b = attachment->b;
            a = attachment->a;
            break;
                                 }
        case SP_ATTACHMENT_SKINNED_MESH: {
            spSkinnedMeshAttachment* attachment = (spSkinnedMeshAttachment*)slot->attachment;
            spSkinnedMeshAttachment_computeWorldVertices(attachment, slot, _worldVertices);
            texture = getTexture(attachment);
            uvs = attachment->uvs;
            verticesCount = attachment->uvsCount;
            triangles = attachment->triangles;
            trianglesCount = attachment->trianglesCount;
            r = attachment->r;
            g = attachment->g;
            b = attachment->b;
            a = attachment->a;
            break;
                                         }
        default: ;
        } 
        if (texture) {
            if (slot->data->additiveBlending != additive) {
                _batch->flush();
                GL::blendFunc(_blendFunc.src, slot->data->additiveBlending ? GL_ONE : _blendFunc.dst);
                additive = slot->data->additiveBlending;
            }
            color.a = _skeleton->a * slot->a * a * 255;
            float multiplier = _premultipliedAlpha ? color.a : 255;
            color.r = _skeleton->r * slot->r * r * multiplier;
            color.g = _skeleton->g * slot->g * g * multiplier;
            color.b = _skeleton->b * slot->b * b * multiplier;
            _batch->add(texture, _worldVertices, uvs, verticesCount, triangles, trianglesCount, &color);
        }
    }
    _batch->flush();
}

void MySkeletonAnimation::drawDebug (const cocos2d::Mat4& transform, uint32_t transformFlags) {
    Director* director = Director::getInstance();
    director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, transform);

    if (_debugSlots) {
        // Slots.
        DrawPrimitives::setDrawColor4B(0, 0, 255, 255);
        glLineWidth(1);
        Vec2 points[4];
        V3F_C4B_T2F_Quad quad;
        for (int i = 0, n = _skeleton->slotsCount; i < n; i++) {
            spSlot* slot = _skeleton->drawOrder[i];
            if (!slot->attachment || slot->attachment->type != SP_ATTACHMENT_REGION) continue;
            spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
            spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
            points[0] = Vec2(_worldVertices[0], _worldVertices[1]);
            points[1] = Vec2(_worldVertices[2], _worldVertices[3]);
            points[2] = Vec2(_worldVertices[4], _worldVertices[5]);
            points[3] = Vec2(_worldVertices[6], _worldVertices[7]);
            DrawPrimitives::drawPoly(points, 4, true);
        }
    }
    if (_debugBones) {
        // Bone lengths.
        glLineWidth(2);
        DrawPrimitives::setDrawColor4B(255, 0, 0, 255);
        for (int i = 0, n = _skeleton->bonesCount; i < n; i++) {
            spBone *bone = _skeleton->bones[i];
            float x = bone->data->length * bone->m00 + bone->worldX;
            float y = bone->data->length * bone->m10 + bone->worldY;
            DrawPrimitives::drawLine(Vec2(bone->worldX, bone->worldY), Vec2(x, y));
        }
        // Bone origins.
        DrawPrimitives::setPointSize(4);
        DrawPrimitives::setDrawColor4B(0, 0, 255, 255); // Root bone is blue.
        for (int i = 0, n = _skeleton->bonesCount; i < n; i++) {
            spBone *bone = _skeleton->bones[i];
            DrawPrimitives::drawPoint(Vec2(bone->worldX, bone->worldY));
            if (i == 0) DrawPrimitives::setDrawColor4B(0, 255, 0, 255);
        }
    }
    director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

bool MySkeletonAnimation::hasBone(const std::string& boneName) const {
    return findBone(boneName) != nullptr;
}

bool MySkeletonAnimation::hasAnimation(const std::string& name) const {
    spAnimation* animation = spSkeletonData_findAnimation(_skeleton->data, name.c_str());
    return animation != 0;
}

Vec2 MySkeletonAnimation::getBoneWorldPos(const std::string& boneName) const {
    auto bone = findBone(boneName);
    if (bone) {
        return cocos2d::Vec2(bone->worldX, bone->worldY);
    }
    return cocos2d::Vec2();
}

cocos2d::Rect MySkeletonAnimation::getInnerBoundingBox() const {
    auto rect = getBoundingBox();
    rect.origin.x -= _position.x;
    rect.origin.y -= _position.y;
    return rect;
}

Node* MySkeletonAnimation::getSlotNode(const std::string& slotName) {
    auto slot = findSlot(slotName);
    if (!slot) return nullptr;

    auto iter = _slotNodeMap.find(slot);
    if (iter != _slotNodeMap.end()) {
        return iter->second;
    }
    return nullptr;
}
Node* MySkeletonAnimation::bindSlotNode(const std::string& slotName, cocos2d::Node *node) {
    auto slot = findSlot(slotName);
    if (!slot) return nullptr;

    auto iter = _slotNodeMap.find(slot);
    if (iter != _slotNodeMap.end()) {
        return iter->second;
    }

    if (!node) {
        node = Node::create();
    }
    node->setVisible(false);
    addChild(node);
    _slotNodeMap[slot] = node;

    return node;
}
void MySkeletonAnimation::unbindSlotNode(const std::string& slotName) {
    auto slot = findSlot(slotName);
    if (!slot) return;

    auto iter = _slotNodeMap.find(slot);
    if (iter == _slotNodeMap.end()) return;

    auto node = iter->second;
    if (node->getParent() == this) {
        node->removeFromParent();
    }
}

}
