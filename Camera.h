#pragma once

#include "Math.h"

using Vector3 = glm::vec3;
using Quaternion = glm::quat;
using Matrix4 = glm::mat4;

//-----------------------------------------------------------------------------
// A general purpose 6DoF (six degrees of freedom) quaternion based camera.
//
// This camera class supports 4 different behaviors:
// first person mode, spectator mode, flight mode, and orbit mode.
//
// First person mode only allows 5DOF (x axis movement, y axis movement, z axis
// movement, yaw, and pitch) and movement is always parallel to the world x-z
// (ground) plane.
//
// Spectator mode is similar to first person mode only movement is along the
// direction the camera is pointing.
// 
// Flight mode supports 6DoF. This is the camera class' default behavior.
//
// Orbit mode rotates the camera around a target position. This mode can be
// used to simulate a third person camera. Orbit mode supports 2 modes of
// operation: orbiting about the target's Y axis, and free orbiting. The former
// mode only allows pitch and yaw. All yaw changes are relative to the target's
// local Y axis. This means that camera yaw changes won't be affected by any
// rolling. The latter mode allows the camera to freely orbit the target. The
// camera is free to pitch, yaw, and roll. All yaw changes are relative to the
// camera's orientation (in space orbiting the target).
//
// This camera class allows the camera to be moved in 2 ways: using fixed
// step world units, and using a supplied velocity and acceleration. The former
// simply moves the camera by the specified amount. To move the camera in this
// way call one of the move() methods. The other way to move the camera
// calculates the camera's displacement based on the supplied velocity,
// acceleration, and elapsed time. To move the camera in this way call the
// updatePosition() method.
//-----------------------------------------------------------------------------

class Camera final
{
public:
    enum class CameraBehavior
    {
        CAMERA_BEHAVIOR_FIRST_PERSON,
        CAMERA_BEHAVIOR_SPECTATOR,
        CAMERA_BEHAVIOR_FLIGHT,
        CAMERA_BEHAVIOR_ORBIT
    };

    void lookAt(const Vector3 &target);
    void lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up);
    void move(float dx, float dy, float dz);
    void move(const Vector3 &direction, const Vector3 &amount);
    void perspective(float fovx, float aspect, float znear, float zfar, bool fovy = false);
    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);
    void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees);
    void undoRoll();
    void updatePosition(const Vector3 &direction, float elapsedTimeSec);
    void zoom(float zoom, float minZoom, float maxZoom);

    // Getter methods.
    inline const Vector3 &getAcceleration() const { return m_acceleration; }
    inline Camera::CameraBehavior getBehavior() const { return m_behavior; }
    inline const Vector3 &getCurrentVelocity() const { return m_currentVelocity; }
    inline const Vector3 &getPosition() const { return m_eye; }
    inline float getOrbitMinZoom() const { return m_orbitMinZoom; }
    inline float getOrbitMaxZoom() const { return m_orbitMaxZoom; }
    inline float getOrbitOffsetDistance() const { return m_orbitOffsetDistance; }
    inline const Quaternion &getOrientation() const { return m_orientation; }
    inline float getRotationSpeed() const { return m_rotationSpeed; }
    inline const Matrix4 &getProjectionMatrix() const { return m_projMatrix; }
    inline const Vector3 &getVelocity() const { return m_velocity; }
    inline const Vector3 &getViewDirection() const { return m_viewDir; }
    inline const Matrix4 &getViewMatrix() const { return m_viewMatrix; }
    inline const Matrix4 &getViewProjectionMatrix() const { return m_viewProjMatrix; }
    inline const Vector3 &getXAxis() const { return m_xAxis; }
    inline const Vector3 &getYAxis() const { return m_yAxis; }
    inline const Vector3 &getZAxis() const { return m_zAxis; }
    inline bool preferTargetYAxisOrbiting() const { return m_preferTargetYAxisOrbiting; }

    
    // Setter methods.

    void setAcceleration(const Vector3 &acceleration);
    void setBehavior(CameraBehavior newBehavior);
    void setCurrentVelocity(const Vector3 &currentVelocity);
    void setCurrentVelocity(float x, float y, float z);
    void setOrbitMaxZoom(float orbitMaxZoom);
    void setOrbitMinZoom(float orbitMinZoom);
    void setOrbitOffsetDistance(float orbitOffsetDistance);
    void setOrbitPitchMaxDegrees(float orbitPitchMaxDegrees);
    void setOrbitPitchMinDegrees(float orbitPitchMinDegrees);
    void setOrientation(const Quaternion &newOrientation);
    void setPosition(const Vector3 &newEye);
    void setPreferTargetYAxisOrbiting(bool preferTargetYAxisOrbiting);
    void setRotationSpeed(float rotationSpeed);
    void setVelocity(const Vector3 &velocity);
    void setVelocity(float x, float y, float z);
    
    static constexpr float DEFAULT_ROTATION_SPEED = 0.3f;
    static constexpr float DEFAULT_FOVX = 90.0f;
    static constexpr float DEFAULT_ZNEAR = 0.1f;
    static constexpr float DEFAULT_ZFAR = 1000.0f;

    static constexpr float DEFAULT_ORBIT_MIN_ZOOM = DEFAULT_ZNEAR + 1.0f;
    static constexpr float DEFAULT_ORBIT_MAX_ZOOM = DEFAULT_ZFAR * 0.5f;

    static constexpr float DEFAULT_ORBIT_OFFSET_DISTANCE = DEFAULT_ORBIT_MIN_ZOOM +
        (DEFAULT_ORBIT_MAX_ZOOM - DEFAULT_ORBIT_MIN_ZOOM) * 0.25f;

private:
    void rotateFirstPerson(float headingDegrees, float pitchDegrees);
    void rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees);
    void rotateOrbit(float headingDegrees, float pitchDegrees, float rollDegrees);
    void updateVelocity(const Vector3 &direction, float elapsedTimeSec);
    void updateViewMatrix();

    static inline constexpr Vector3 WORLD_XAXIS { 1.0f, 0.0f, 0.0f };
    static inline constexpr Vector3 WORLD_YAXIS { 0.0f, 1.0f, 0.0f };
    static inline constexpr Vector3 WORLD_ZAXIS { 0.0f, 0.0f, 1.0f };

    CameraBehavior m_behavior = CameraBehavior::CAMERA_BEHAVIOR_FLIGHT;
    bool m_preferTargetYAxisOrbiting = true;
    float m_accumPitchDegrees = 0.0f;
    float m_savedAccumPitchDegrees = 0.0f;
    float m_rotationSpeed = DEFAULT_ROTATION_SPEED;
    float m_fovx = DEFAULT_FOVX;
    float m_aspectRatio = 0.0f;
    float m_znear = DEFAULT_ZNEAR;
    float m_zfar = DEFAULT_ZFAR;
    float m_orbitMinZoom = DEFAULT_ORBIT_MIN_ZOOM;
    float m_orbitMaxZoom = DEFAULT_ORBIT_MAX_ZOOM;
    float m_orbitOffsetDistance = DEFAULT_ORBIT_OFFSET_DISTANCE;
    float m_firstPersonYOffset = 0.0f;
    Vector3 m_eye = {};
    Vector3 m_savedEye = {};
    Vector3 m_target = {};
    Vector3 m_targetYAxis = { 0.0f, 1.0f, 0.0f };
    Vector3 m_xAxis = { 1.0f, 0.0f, 0.0f };
    Vector3 m_yAxis = { 0.0f, 1.0f, 0.0f };
    Vector3 m_zAxis = { 0.0f, 0.0f, 1.0f };
    Vector3 m_viewDir = -m_zAxis;
    Vector3 m_acceleration = {};
    Vector3 m_currentVelocity = {};
    Vector3 m_velocity = {};
    Quaternion m_orientation = {};
    Quaternion m_savedOrientation = {};
    Matrix4 m_viewMatrix = {};
    Matrix4 m_projMatrix = {};
    Matrix4 m_viewProjMatrix = {};
};
