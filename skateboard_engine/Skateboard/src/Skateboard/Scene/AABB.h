#pragma once
#include <string>
#include "Skateboard/Graphics/InternalFormats.h"
#include "Skateboard/Mathematics.h"

namespace Skateboard
{
	struct AABB
	{
		union
		{
			struct {
				float MinX;
				float MinY;
				float MinZ;
				float MaxX;
				float MaxY;
				float MaxZ;
			};
			struct {
				float3 Min;
				float3 Max;
			};
			struct
			{
				float aabb[6];
			};
		};
	};

	struct BoundingSphere
	{
		float3 Center;
		float Radius;
	};

	struct RaytracingAABBDesc
	{
		std::wstring Name;
		GeometryType_ Type;
		AABB BoundingBox;
	};
}