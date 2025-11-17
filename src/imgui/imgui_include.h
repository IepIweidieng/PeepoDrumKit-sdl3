#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_application_host.h"
#include "imgui_custom_draw.h"
#include "extension/imgui_common.h"
#include "extension/imgui_input_binding.h"

// NOTE: This should be the only public Dear Imgui header included by all other src files (except for 3rdparty_extension to prevent include polution)
//		 All external code should only reference the ImGui namespace using this alias
namespace Gui = ImGui;
