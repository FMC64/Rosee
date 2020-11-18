#pragma once

#include "Vk.hpp"

namespace Rosee {

class Renderer
{
	bool m_validate;
	bool m_use_render_doc;
	Vk::Instance m_instance;
	Vk::Instance createInstance(void);

public:
	Renderer(bool validate, bool useRenderDoc);
};

}