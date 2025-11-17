#include "imgui_custom_draw.h"
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>

struct WAVEFORM_CONSTANT_BUFFER
{
	struct { vec2 Pos, UV; } PerVertex[6];
	struct { vec2 Size, SizeInv; } CB_RectSize;
	struct { f32 R, G, B, A; } Color;
	float Amplitudes[CustomDraw::WaveformPixelsPerChunk];
};

namespace CustomDraw
{
    static ImGui_ImplDX11_RenderState* ImGui_ImplDX11_GetBackendData()
    {
        if (ImGui::GetCurrentContext()) {
            auto state = ImGui::GetPlatformIO().Renderer_RenderState;
            return static_cast<ImGui_ImplDX11_RenderState*>(state);
        } else {
            return nullptr;
        }
    }

	struct DX11GPUTextureData
	{
		// NOTE: An ID of 0 denotes an empty slot
		u32 GenerationID;
		GPUTextureDesc Desc;
		ID3D11Texture2D* Texture2D;
		ID3D11ShaderResourceView* ResourceView;
	};

	// NOTE: First valid ID starts at 1
	static u32 LastTextureGenreationID;
	static std::vector<DX11GPUTextureData> LoadedTextureSlots;
	static std::vector<ID3D11DeviceChild*> DeviceResourcesToDeferRelease;

	inline DX11GPUTextureData* ResolveHandle(GPUTextureHandle handle)
	{
		auto& slots = LoadedTextureSlots;
		return (handle.GenerationID != 0) && (handle.SlotIndex < slots.size()) && (slots[handle.SlotIndex].GenerationID == handle.GenerationID) ? &slots[handle.SlotIndex] : nullptr;
	}

	void GPUTexture::Load(const GPUTextureDesc& desc)
	{
		auto bd = ImGui_ImplDX11_GetBackendData();
        if (!bd) {
            std::cerr << "Error: ImGui DX11 backend data is null in GPUTexture::Load\n";
            return;
        }
		assert(ResolveHandle(Handle) == nullptr);

		DX11GPUTextureData* slot = nullptr;
		for (auto& it : LoadedTextureSlots) { if (it.GenerationID == 0) { slot = &it; break; } }
		if (slot == nullptr) { slot = &LoadedTextureSlots.emplace_back(); }

		slot->GenerationID = ++LastTextureGenreationID;
		slot->Desc = desc;

		HRESULT result = bd->Device->CreateTexture2D(PtrArg(D3D11_TEXTURE2D_DESC
			{
				static_cast<UINT>(desc.Size.x), static_cast<UINT>(desc.Size.y), 1u, 1u,
				(desc.Format == GPUPixelFormat::RGBA) ? DXGI_FORMAT_R8G8B8A8_UNORM : (desc.Format == GPUPixelFormat::BGRA) ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_UNKNOWN,
				DXGI_SAMPLE_DESC { 1u, 0u },
				(desc.Access == GPUAccessType::Dynamic) ? D3D11_USAGE_DYNAMIC : (desc.Access == GPUAccessType::Static) ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT,
				D3D11_BIND_SHADER_RESOURCE,
				(desc.Access == GPUAccessType::Dynamic) ? D3D11_CPU_ACCESS_WRITE : 0u, 0u
			}),
			PtrArg(D3D11_SUBRESOURCE_DATA{ desc.InitialPixels, static_cast<UINT>(desc.Size.x * 4), 0u }),
			//(desc.InitialPixels != nullptr) ? PtrArg(D3D11_SUBRESOURCE_DATA { desc.InitialPixels, static_cast<UINT>(desc.Size.x * 4), 0u }) : nullptr,
			&slot->Texture2D);
		assert(SUCCEEDED(result));

		result = bd->Device->CreateShaderResourceView(slot->Texture2D, nullptr, &slot->ResourceView);
		assert(SUCCEEDED(result));

		Handle = GPUTextureHandle { static_cast<u32>(ArrayItToIndex(slot, &LoadedTextureSlots[0])), slot->GenerationID };
	}

	void GPUTexture::Unload()
	{
		if (auto* data = ResolveHandle(Handle); data != nullptr)
		{
			DeviceResourcesToDeferRelease.push_back(data->Texture2D);
			DeviceResourcesToDeferRelease.push_back(data->ResourceView);
			*data = DX11GPUTextureData {};
		}
		Handle = GPUTextureHandle {};
	}

	void GPUTexture::UpdateDynamic(ivec2 size, const void* newPixels)
	{
		auto* data = ResolveHandle(Handle);
		if (data == nullptr)
			return;

		assert(data->Desc.Access == GPUAccessType::Dynamic);
		assert(data->Desc.Size == size);
		auto bd = ImGui_ImplDX11_GetBackendData();
        if (!bd) {
            std::cerr << "Error: ImGui DX11 backend data is null in GPUTexture::UpdateDynamic\n";
            return;
        }

		if (D3D11_MAPPED_SUBRESOURCE mapped; SUCCEEDED(bd->DeviceContext->Map(data->Texture2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			const size_t outStride = mapped.RowPitch;
			const size_t outByteSize = mapped.DepthPitch;
			u8* outData = static_cast<u8*>(mapped.pData);
			assert(outData != nullptr);

			assert(data->Desc.Format == GPUPixelFormat::RGBA || data->Desc.Format == GPUPixelFormat::BGRA);
			static constexpr u32 rgbaBitsPerPixel = (sizeof(u32) * BitsPerByte);
			const size_t inStride = (size.x * rgbaBitsPerPixel) / BitsPerByte;
			const size_t inByteSize = (size.y * inStride);
			const u8* inData = static_cast<const u8*>(newPixels);

			if (outByteSize == inByteSize)
			{
				memcpy(outData, inData, inByteSize);
			}
			else
			{
				assert(outByteSize == (outStride * size.x));
				for (size_t y = 0; y < size.y; y++)
					memcpy(&outData[outStride * y], &inData[inStride * y], inStride);
			}

			bd->DeviceContext->Unmap(data->Texture2D, 0);
		}
	}

	b8 GPUTexture::IsValid() const { return (ResolveHandle(Handle) != nullptr); }
	ivec2 GPUTexture::GetSize() const { auto* data = ResolveHandle(Handle); return data ? data->Desc.Size : ivec2(0, 0); }
	vec2 GPUTexture::GetSizeF32() const { return vec2(GetSize()); }
	GPUPixelFormat GPUTexture::GetFormat() const { auto* data = ResolveHandle(Handle); return data ? data->Desc.Format : GPUPixelFormat {}; }
	ImTextureID GPUTexture::GetTexID() const { auto* data = ResolveHandle(Handle); return data ? (ImTextureID)data->ResourceView : 0; }

	// NOTE: The most obvious way to extend this would be to either add an enum command type + a union of parameters
	//		 or (better?) a per command type commands vector with the render callback userdata storing a packed type+index
	struct DX11CustomDrawCommand
	{
		Rect Rect;
		ImVec4 Color;
		CustomDraw::WaveformChunk WaveformChunk;
	};

	static ImDrawData* ThisFrameImDrawData = nullptr;
	static std::vector<DX11CustomDrawCommand> CustomDrawCommandsThisFrame;

	static void DX11RenderInit(ImGui_ImplDX11_RenderState* bd)
	{
		CustomDrawCommandsThisFrame.reserve(64);
		LoadedTextureSlots.reserve(8);
		DeviceResourcesToDeferRelease.reserve(LoadedTextureSlots.capacity() * 2);
	}

	static void DX11BeginRenderDrawData(ImDrawData* drawData)
	{
		ThisFrameImDrawData = drawData;
	}

	static void DX11EndRenderDrawData(ImDrawData* drawData)
	{
		assert(drawData == ThisFrameImDrawData);
		ThisFrameImDrawData = nullptr;
		CustomDrawCommandsThisFrame.clear();
	}

	static void DX11ReleaseDeferedResources(ImGui_ImplDX11_RenderState* bd)
	{
		assert(bd != nullptr && bd->Device != nullptr);

		if (!DeviceResourcesToDeferRelease.empty())
		{
			for (ID3D11DeviceChild* it : DeviceResourcesToDeferRelease) { if (it != nullptr) it->Release(); }
			DeviceResourcesToDeferRelease.clear();
		}
	}

	void DrawWaveformChunk(ImDrawList* drawList, Rect rect, u32 color, const WaveformChunk& chunk)
	{
		// ImDrawCallback callback = [](const ImDrawList* parentList, const ImDrawCmd* cmd)
		// {
		// 	static_assert(sizeof(WAVEFORM_CONSTANT_BUFFER::Amplitudes) == sizeof(WaveformChunk::PerPixelAmplitude));
		// 	const DX11CustomDrawCommand& customCommand = CustomDrawCommandsThisFrame[reinterpret_cast<size_t>(cmd->UserCallbackData)];

		// 	ImGui_ImplDX11_RenderState* bd = ImGui_ImplDX11_GetBackendData();
		// 	ID3D11DeviceContext* ctx = bd->DeviceContext;

		// 	if (D3D11_MAPPED_SUBRESOURCE mapped; ctx->Map(bd->WaveformConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK)
		// 	{
		// 		constexpr vec2 uvTL = vec2(0.0f, 0.0f); constexpr vec2 uvTR = vec2(1.0f, 0.0f);
		// 		constexpr vec2 uvBL = vec2(0.0f, 1.0f); constexpr vec2 uvBR = vec2(1.0f, 1.0f);

		// 		WAVEFORM_CONSTANT_BUFFER* data = static_cast<WAVEFORM_CONSTANT_BUFFER*>(mapped.pData);
		// 		data->PerVertex[0] = { customCommand.Rect.GetBL(), uvBL };
		// 		data->PerVertex[1] = { customCommand.Rect.GetBR(), uvBR };
		// 		data->PerVertex[2] = { customCommand.Rect.GetTR(), uvTR };
		// 		data->PerVertex[3] = { customCommand.Rect.GetBL(), uvBL };
		// 		data->PerVertex[4] = { customCommand.Rect.GetTR(), uvTR };
		// 		data->PerVertex[5] = { customCommand.Rect.GetTL(), uvTL };
		// 		data->CB_RectSize = { customCommand.Rect.GetSize(), vec2(1.0f) / customCommand.Rect.GetSize() };
		// 		data->Color = { customCommand.Color.x, customCommand.Color.y, customCommand.Color.z, customCommand.Color.w };
		// 		memcpy(&data->Amplitudes, &customCommand.WaveformChunk.PerPixelAmplitude, sizeof(data->Amplitudes));
		// 		ctx->Unmap(bd->WaveformConstantBuffer, 0);
		// 	}

		// 	// HACK: Duplicated from regular render command loop
		// 	ImVec2 clip_off = ThisFrameImDrawData->DisplayPos;
		// 	ImVec2 clip_min(cmd->ClipRect.x - clip_off.x, cmd->ClipRect.y - clip_off.y);
		// 	ImVec2 clip_max(cmd->ClipRect.z - clip_off.x, cmd->ClipRect.w - clip_off.y);
		// 	if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
		// 		return;

		// 	const D3D11_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
		// 	ctx->RSSetScissorRects(1, &r);

		// 	ctx->VSSetShader(bd->WaveformVS, nullptr, 0);
		// 	ctx->PSSetShader(bd->WaveformPS, nullptr, 0);
		// 	ctx->Draw(6, 0);

		// 	// HACK: Always reset state for now even if it's immediately set back by the next render command
		// 	ctx->VSSetShader(bd->pVertexShader, nullptr, 0);
		// 	ctx->PSSetShader(bd->pPixelShader, nullptr, 0);
		// };

		// void* userData = reinterpret_cast<void*>(CustomDrawCommandsThisFrame.size());
		// CustomDrawCommandsThisFrame.push_back(DX11CustomDrawCommand { rect, ImGui::ColorConvertU32ToFloat4(color), chunk });

		// drawList->AddCallback(callback, userData);
	}
}
