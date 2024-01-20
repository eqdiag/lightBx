#pragma once

#include "math/vec.h"
#include "math/matrix.h"

namespace vk_primitives{

    namespace camera {
        
        static const double CAMERA_SHIFT = 0.001;

        class Camera {
        public:
            Camera(
                float theta,
                float phi,
                const math::Vec3& eye,
                const math::Vec3& up,
                const math::Vec3& forward,
                const math::Vec3& right
            );

            virtual void translate(float x, float y, float z) = 0;
            virtual void rotateTheta(float dtheta) = 0;
            virtual void rotatePhi(float dphi) = 0;
            virtual void zoom(float amt) = 0;
            virtual void setRadius(double radius) = 0;
            virtual void reset(math::Vec3& center, double radius) = 0;

            virtual math::Mat4 getViewMatrix() const = 0;
            virtual math::Vec3 getEye() const = 0;

            virtual void recomputeFrame() = 0;

        protected:

            //Spherical coordinate representation
            float mTheta;
            float mPhi;

            //Orthonormal basis representation
            math::Vec3 mEye;
            math::Vec3 mUp;
            math::Vec3 mForward;
            math::Vec3 mRight;
        };

        class FlyCamera : public Camera {
        public:
            FlyCamera(const math::Vec3& eye);
            void translate(float x, float y, float z) override;
            void rotateTheta(float dtheta) override;
            void rotatePhi(float dphi) override;
            void zoom(float amt) override;
            void setRadius(double radius) override;
            void reset(math::Vec3& center, double radius) override;

            math::Mat4 getViewMatrix() const override;
            math::Vec3 getEye() const override;

        private:

            static const double MIN_THETA;
            static const double MAX_THETA;
            static const double MIN_PHI;
            static const double MAX_PHI;

            void recomputeFrame() override;

        };

        class ArcCamera : public Camera {

        public:
            ArcCamera(math::Vec3& center, float radius);
            void translate(float x, float y, float z) override;
            void rotateTheta(float dtheta) override;
            void rotatePhi(float dphi) override;
            void zoom(float amt) override;
            void setRadius(double radius) override;
            void reset(math::Vec3& center, double radius) override;

            math::Mat4 getViewMatrix() const override;
            math::Vec3 getEye() const override;

        private:
            void recomputeFrame() override;

            static const double MIN_PHI;
            static const double MAX_PHI;

            math::Vec3 mCenter;
            float mRadius;
        };

    }
}

