
#include "AudioRenderer.h"
#include "sktbdpch.h"


namespace Skateboard
{
	AudioRendererAPI* AudioRenderer::m_AudioRendererAPI{ nullptr };

	void AudioRenderer::End()
	{
		m_AudioRendererAPI->EndScene();
	}

	void AudioRenderer::LoadAudio(char* path, char* ID)
	{
		m_AudioRendererAPI->LoadAudio(path, ID);
	}

	void AudioRenderer::LoadBank(const char* path, const char* ID)
	{
		m_AudioRendererAPI->LoadBank(path, ID);
	}
	SkbdSoundHandle AudioRenderer::PlayAudio(const char *ID, SkbdSoundParams params )
	{
		return m_AudioRendererAPI->PlayAudio(ID, params);
	}

	SkbdSoundHandle AudioRenderer::StopAudio(SkbdSoundHandle hnd)
	{
		return m_AudioRendererAPI->StopAudio(hnd);
	}

	SkbdSoundHandle AudioRenderer::PauseAudio(SkbdSoundHandle hnd)
	{
		return m_AudioRendererAPI->PauseAudio(hnd);
	}

	SkbdSoundHandle AudioRenderer::ResumeAudio(SkbdSoundHandle hnd)
	{
		return m_AudioRendererAPI->ResumeAudio(hnd);
	}

	/*void AudioRenderer::UpdateAudioRenderer()
	{
		m_AudioRendererAPI->UpdateAudioRenderer();
	}*/

	void AudioRenderer::AddLocalPlayer(int32_t usrid)
	{
		m_AudioRendererAPI->AddLocalPlayer(usrid);
	}

	void AudioRenderer::RemoveLocalPlayer(int32_t usrid)
	{
		m_AudioRendererAPI->RemoveLocalPlayer(usrid);
	}

	void AudioRenderer::SetGroupVolume(float vol, int32_t group)
	{
		m_AudioRendererAPI->SetGroupVolume(group, vol);
	}

	void AudioRenderer::SetGlobalVariable(const char* name, float value)
	{
		m_AudioRendererAPI->SetGlobalVariable(name, value);
	}

	void AudioRenderer::SetGlobalRegister(int reg, int val)
	{
		m_AudioRendererAPI->SetGlobalRegister(reg, val);
	}

}