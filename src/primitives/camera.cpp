#include "camera.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TPI 2.0*M_PI


const double vk_primitives::camera::FlyCamera::MIN_THETA = 0.0;
const double vk_primitives::camera::FlyCamera::MAX_THETA = TPI;
const double vk_primitives::camera::FlyCamera::MIN_PHI = 0.0;
const double vk_primitives::camera::FlyCamera::MAX_PHI = M_PI;

const double vk_primitives::camera::ArcCamera::MIN_PHI = 0.0001;
const double vk_primitives::camera::ArcCamera::MAX_PHI = M_PI - 0.0001;

vk_primitives::camera::Camera::Camera(float theta, float phi, const math::Vec3& eye, const math::Vec3& up, const math::Vec3& forward, const math::Vec3& right)
    :mTheta{ theta }, mPhi{ phi }, mEye{ eye }, mUp{ up }, mForward{ forward }, mRight{ right }
{
}

vk_primitives::camera::FlyCamera::FlyCamera(const math::Vec3& eye) :
    Camera{
        static_cast<float>(0.5 * M_PI),
        static_cast <float>(0.5 * M_PI),
        eye,
        math::Vec3(0.0,1.0,0.0),
        math::Vec3(0.0,0.0,-1.0),
        math::Vec3(1.0,0.0,0.0)
}
{
    recomputeFrame();
}

void vk_primitives::camera::FlyCamera::translate(float x, float y, float z) {
    mEye += mRight * x + mUp * y - mForward * z;
}

void vk_primitives::camera::FlyCamera::rotateTheta(float dtheta) {
    mTheta += dtheta;
    recomputeFrame();
}

void vk_primitives::camera::FlyCamera::rotatePhi(float dphi) {
    mPhi += dphi;
    mPhi = std::clamp(mPhi, static_cast<float>(MIN_PHI), static_cast<float>(MAX_PHI));
    recomputeFrame();
}

void vk_primitives::camera::FlyCamera::zoom(float amt)
{

}

void vk_primitives::camera::FlyCamera::setRadius(double radius)
{
    radius = radius;
    recomputeFrame();
}

void vk_primitives::camera::FlyCamera::reset(math::Vec3& center, double radius)
{
    mTheta = static_cast<float>(0.5 * M_PI);
    mPhi = static_cast <float>(0.5 * M_PI);
    mEye = center;
    mUp = math::Vec3(0.0, 1.0, 0.0);
    mForward = math::Vec3(0.0, 0.0, -1.0);
    mRight = math::Vec3(1.0, 0.0, 0.0);
}

math::Mat4 vk_primitives::camera::FlyCamera::getViewMatrix() const
{
    math::Vec3 center = mEye + mForward;
    return math::Mat4::lookAt(mEye, center, mUp);
}

math::Vec3 vk_primitives::camera::FlyCamera::getEye() const {
    return mEye;
}


void vk_primitives::camera::FlyCamera::recomputeFrame() {
    float sp = sin(mPhi);
    float cp = cos(mPhi);
    float st = sin(mTheta);
    float ct = cos(mTheta);
    mForward = math::Vec3(sp * ct, cp, -sp * st);

    mRight = mForward.cross(mUp).normalize();
}



vk_primitives::camera::ArcCamera::ArcCamera(math::Vec3& center, float radius) :
    Camera{
        static_cast <float>(0.5 * M_PI),
        static_cast <float>(0.5 * M_PI),
        center - math::Vec3(0.0,0.0,-1.0) * radius,
        math::Vec3(0.0,1.0,0.0),
        math::Vec3(0.0,0.0,-1.0),
        math::Vec3(1.0,0.0,0.0)
},
mCenter{ center },
mRadius{ radius }
{
    recomputeFrame();
}


void vk_primitives::camera::ArcCamera::translate(float x, float y, float z) {

}


void vk_primitives::camera::ArcCamera::rotateTheta(float dtheta) {
    mTheta += dtheta;
    recomputeFrame();
}

void vk_primitives::camera::ArcCamera::rotatePhi(float dphi) {
    mPhi += dphi;
    mPhi = std::clamp(mPhi, static_cast<float>(MIN_PHI), static_cast<float>(MAX_PHI));
    recomputeFrame();
}

void vk_primitives::camera::ArcCamera::zoom(float amt) {
    mRadius += amt;
    if (mRadius < 0.0) mRadius = 0.;
    recomputeFrame();
}

void vk_primitives::camera::ArcCamera::setRadius(double radius)
{
    radius = radius;
    recomputeFrame();
}

void vk_primitives::camera::ArcCamera::reset(math::Vec3& center, double radius)
{
    this->mCenter = center;
    this->mRadius = radius;
    mTheta = static_cast <float>(0.5 * M_PI);
    mPhi = static_cast <float>(0.5 * M_PI);
    recomputeFrame();
}

math::Mat4 vk_primitives::camera::ArcCamera::getViewMatrix() const {
    return math::Mat4::lookAt(mEye, mCenter, mUp);
}

math::Vec3 vk_primitives::camera::ArcCamera::getEye() const {
    return mEye;
}

void vk_primitives::camera::ArcCamera::recomputeFrame() {
    float sp = sin(mPhi);
    float cp = cos(mPhi);
    float st = sin(mTheta);
    float ct = cos(mTheta);
    mEye = -math::Vec3(sp * ct, cp, -sp * st) * mRadius + mCenter;
    mForward = (mCenter - mEye).normalize();
    mRight = mForward.cross(mUp).normalize();
}

