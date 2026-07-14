#pragma once

#include <DirectXMath.h>

#include <Glacier/ZMath.hpp>

#include "Common.hpp"
#include "IDirectXTKRenderer.hpp"

namespace zknt::rendering {
    class ViewFrustum {
      public:
        void UpdateClipPlanes(const SMatrix& p_View, const SMatrix& p_Projection);
        bool ContainsPoint(const SVector3& p_Point) const;
        bool ContainsAABB(const AABB& p_AABB) const;
        bool ContainsOBB(const SMatrix& p_Transform, const float4& p_Center, const float4& p_HalfSize) const;

        void SetDistanceCullingEnabled(bool p_Enabled);
        bool IsDistanceCullingEnabled() const;

        void SetMaxDrawDistance(float p_MaxDrawDistance);
        float GetMaxDrawDistance() const;

      private:
        enum class ECheckInsideFlag { CHECK_INSIDE_FULLY_OUTSIDE, CHECK_INSIDE_PARTIALLY_INSIDE, CHECK_INSIDE_FULLY_INSIDE, CHECK_INSIDE_UNKNOWN };

        SMatrix MatrixPerspectiveFovRH(float p_FovYDeg, float p_AspectWByH, float p_NearZ, float p_FarZ);
        SMatrix MatrixPerspectiveRH(float p_Width, float p_Height, float p_NearZ, float p_FarZ);
        void MatrixCreateClipPlanes(float4* p_Planes, const SMatrix& p_ViewProjection);
        void MatrixCreateClipPlanesNormalized(float4* p_Planes, const SMatrix& p_ViewProjection);

        ECheckInsideFlag CheckPointInsidePlanes(const SVector3& p_Point) const;
        ECheckInsideFlag CheckAABBInsidePlanes(const AABB& p_AABB) const;
        ECheckInsideFlag CheckOBBInsidePlanes(const SMatrix& p_ObjectInternal, const float4& p_LocalCenter, const float4& p_LocalSize) const;

        bool m_IsDistanceCullingEnabled = false;
        float m_MaxDrawDistance = 50.f;
        SMatrix m_ClipPlaneProjectionMatrix;
        float4 m_Planes[6];
        float m_FovYDeg = 0.f;
        float m_AspectWByH = 0.f;
        float m_NearZ = 0.f;
        float m_FarZ = -1.f;
    };
}
