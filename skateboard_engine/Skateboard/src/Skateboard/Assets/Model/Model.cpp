#include "sktbdpch.h"
#include "Model.h"

#define SKTBD_LOG_COMPONENT "MODEL"
#include "Skateboard/Log.h"

namespace Skateboard
{
	Mesh::Mesh(const wchar_t* filename)
	{

	}

	Mesh::Mesh(const std::vector<Primitive>& meshes)
		:
		m_Primitives(meshes)
	{}
}
