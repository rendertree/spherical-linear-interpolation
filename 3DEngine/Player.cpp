// Copyright (c) 2024 Wildan R Wijanarko (@wildan9)

// This software is provided "as-is", without any express or implied warranty. In no event 
// will the authors be held liable for any damages arising from the use of this software.

// Permission is granted to anyone to use this software for any purpose, including commercial 
// applications, and to alter it and redistribute it freely, subject to the following restrictions:

//   1. The origin of this software must not be misrepresented; you must not claim that you 
//   wrote the original software. If you use this software in a product, an acknowledgment 
//   in the product documentation would be appreciated but is not required.

//   2. Altered source versions must be plainly marked as such, and must not be misrepresented
//   as being the original software.

//   3. This notice may not be removed or altered from any source distribution.

#include "Player.h"

Player::Player()
{
    _model = LoadModel("resources/models/mixamo/model1.glb");
    _modelAnimations = LoadModelAnimations("resources/models/mixamo/model1.glb", &_animsCount);
    
    if (_modelAnimations == nullptr)
    {
        TraceLog(LOG_ERROR, "Failed to load model animations");
    }

    _position    = Vector3Zero();
    _destination = Vector3Zero();
    _rotation    = QuaternionIdentity();
    _scale       = Vector3{ 1.0f, 1.0f, 1.0f };

    _model.transform = MatrixTranslateV(_position) * QuaternionToMatrix(QuaternionFromEuler(96.0f, 0.0f, 0.0f)) * MatrixScaleV(_scale);

    _animState = "Idle";

    _animStateActions.insert({ "Idle",        [this](Player& anim) { Idle(anim); } });
    _animStateActions.insert({ "Walk",        [this](Player& anim) { Walk(anim); } });
    _animStateActions.insert({ "Run",         [this](Player& anim) { Run(anim); } });
    _animStateActions.insert({ "Breakdance",  [this](Player& anim) { Breakdance(anim); } });
}

Player::~Player()
{
    UnloadModel(_model);

    if (_modelAnimations != nullptr)
    {
        UnloadModelAnimation(*_modelAnimations);
    }
}

void Player::Idle(Player&)
{
    _animIndex = 1;
}

void Player::Walk(Player&)
{
    _animIndex = 3;
    _movementSpeed = 2.5f;
}

void Player::Run(Player&)
{
    _animIndex = 2;
    _movementSpeed = 7.0f;
}

void Player::Breakdance(Player&)
{
    _animIndex = 0;

    if (_breakdanceDuration > 0)
    {
        _breakdanceDuration -= GetFrameTime();
    }
}

void Player::SetAnimState(const std::string& newState)
{
    auto it = _animStateActions.find(newState);
    if (it != _animStateActions.end())
    {
        if (_animState != newState)
        {
            _animCurrentFrame = 0;
        }
        _animState = newState;
    }
    else
    {
        TRACELOG(LOG_ERROR, "Invalid state");
    }
}

void Player::Update(const Camera& camera)
{
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        Ray ray = GetMouseRay(GetMousePosition(), camera);
        Vector3 groundNormal{ 0.0f, 1.0f, 0.0f };

        float hitDist = -((Vector3DotProduct(ray.position, groundNormal) + 0) / Vector3DotProduct(ray.direction, groundNormal));

        _destination = ray.position + Vector3Scale(ray.direction, hitDist);
    }

    Vector3 direction{};
    if (Vector3Distance(_destination, _position) > 0.1f && _breakdanceDuration < 0.1f)
    {
        direction = Vector3Normalize(_destination - _position);
    }
    else
    {
        direction = Vector3Zero();
    }

    const bool onMove = (Vector3Length(direction) > 0);

    if (onMove && IsKeyDown(KEY_LEFT_SHIFT))
    {
        SetAnimState("Run");
    }
    else if (onMove)
    {
        SetAnimState("Walk");
    }
    else if (_breakdanceDuration > 0)
    {
        SetAnimState("Breakdance");
    }
    else
    {
        SetAnimState("Idle");
    }

    if (onMove)
    {
        float rotationSpeed = 5.0f * GetFrameTime();

        Quaternion currentRotation = _rotation;
        Quaternion targetRotation = QuaternionFromDirection(direction);
        _rotation = QuaternionSlerp(currentRotation, targetRotation, rotationSpeed);
        _position = _position + Vector3Scale(direction, _movementSpeed * GetFrameTime());
    }

    auto action = _animStateActions.find(_animState);
    if (action != _animStateActions.end())
    {
        action->second(*this);
    }

    if (IsKeyPressed(KEY_SPACE) && !onMove)
    {
        _breakdanceDuration = 4.8f;
    }

    const ModelAnimation& anim = _modelAnimations[_animIndex];
    _animCurrentFrame = (_animCurrentFrame + 1) % anim.frameCount;
    UpdateModelAnimation(_model, anim, _animCurrentFrame);
    UpdateTransform3D(_transform, _position, _rotation, _scale);
}

void Player::Draw() const
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(_transform.ToMatrix()));
    DrawModel(_model, Vector3Zero(), 0.02f, WHITE);
    rlPopMatrix();
}