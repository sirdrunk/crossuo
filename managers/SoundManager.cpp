/****************************************************************************
**
** SoundManager.cpp
**
** Copyright (C) September 2015 Hotride
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*****************************************************************************
*/
//---------------------------------------------------------------------------
#include "stdafx.h"

//---------------------------------------------------------------------------
#pragma region BASS.DLL error code descriptions

struct BASS_ErrorDescription
{
	int errorCode;
	char desc[64];
};
//------------------------------------------------------------------------------
BASS_ErrorDescription BASS_ErrorTable[] = {
	{ -2, "unspecified error" },
	{ BASS_OK, "OK" },
	{ BASS_ERROR_MEM, "memory error" },
	{ BASS_ERROR_FILEOPEN, "can't open the file" },
	{ BASS_ERROR_DRIVER, "can't find a free/valid driver" },
	{ BASS_ERROR_BUFLOST, "the sample buffer was lost" },
	{ BASS_ERROR_HANDLE, "invalid handle" },
	{ BASS_ERROR_FORMAT, "unsupported sample format" },
	{ BASS_ERROR_POSITION, "invalid position" },
	{ BASS_ERROR_INIT, "BASS_Init has not been successfully called" },
	{ BASS_ERROR_START, "BASS_Start has not been successfully called" },
	{ BASS_ERROR_SSL, "SSL/HTTPS support isn't available" },
	{ BASS_ERROR_ALREADY, "already initialized/paused/whatever" },
	{ BASS_ERROR_NOCHAN, "can't get a free channel" },
	{ BASS_ERROR_ILLTYPE, "an illegal type was specified" },
	{ BASS_ERROR_ILLPARAM, "an illegal parameter was specified" },
	{ BASS_ERROR_NO3D, "no 3D support" },
	{ BASS_ERROR_NOEAX, "no EAX support" },
	{ BASS_ERROR_DEVICE, "illegal device number" },
	{ BASS_ERROR_NOPLAY, "not playing" },
	{ BASS_ERROR_FREQ, "illegal sample rate" },
	{ BASS_ERROR_NOTFILE, "the stream is not a file stream" },
	{ BASS_ERROR_NOHW, "no hardware voices available" },
	{ BASS_ERROR_EMPTY, "the MOD music has no sequence data" },
	{ BASS_ERROR_NONET, "no internet connection could be opened" },
	{ BASS_ERROR_CREATE, "couldn't create the file" },
	{ BASS_ERROR_NOFX, "effects are not available" },
	{ BASS_ERROR_NOTAVAIL, "requested data is not available" },
	{ BASS_ERROR_DECODE, "the channel is/isn't a \"decoding channel\"" },
	{ BASS_ERROR_DX, "a sufficient DirectX version is not installed" },
	{ BASS_ERROR_TIMEOUT, "connection timedout" },
	{ BASS_ERROR_FILEFORM, "unsupported file format" },
	{ BASS_ERROR_SPEAKER, "unavailable speaker" },
	{ BASS_ERROR_VERSION, "invalid BASS version (used by add-ons)" },
	{ BASS_ERROR_CODEC, "codec is not available/supported" },
	{ BASS_ERROR_ENDED, "the channel/file has ended" },
	{ BASS_ERROR_BUSY, "the device is busy" },
	{ BASS_ERROR_UNKNOWN, "some other mystery problem" },
};
//------------------------------------------------------------------------------
char* BASS_ErrorGetDescription()
{
	auto currentErrorCode = BASS_ErrorGetCode();

	for (auto &bassError : BASS_ErrorTable) {
		if (bassError.errorCode == currentErrorCode)
			return bassError.desc;
	}
	return BASS_ErrorTable[0].desc;
}
#pragma endregion
//---------------------------------------------------------------------------
TSoundManager SoundManager;
//---------------------------------------------------------------------------
TSoundManager::TSoundManager()
: m_Music(0)
{
}
//---------------------------------------------------------------------------
TSoundManager::~TSoundManager()
{
}
//---------------------------------------------------------------------------
bool TSoundManager::Init()
{
	/*if (SDL_Init(SDL_INIT_AUDIO) < 0)
		return false;
	
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 1024) < 0)
	{
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}	*/

	trace_printf("Initializing bass sound system.\n");
	// initialize default output device
	if (!BASS_Init(-1, 48000, BASS_DEVICE_3D, g_hWnd, NULL))
	{
		trace_printf("Can't initialize device: %s\n", BASS_ErrorGetDescription());
		return false;
	} else {
		trace_printf("Sound init successfull.\n");	
		BASS_SetConfig(BASS_CONFIG_SRC, 2); // interpolation method

		if(!BASS_SetConfig(BASS_CONFIG_3DALGORITHM, BASS_3DALG_FULL))
			trace_printf("Error setting 3d sound: %s\n", BASS_ErrorGetDescription());
	}
	
	return true;
}
//---------------------------------------------------------------------------
void TSoundManager::Free()
{
	StopMusic();

	//SDL_CloseAudio();
	//SDL_Quit();

	BASS_Free();
}

/// <summary>������ � ������ 16 ������ wave ���� ��� ������������
/// ������������.</summary>
/// <param name="is">������ �� ������ ����� � MUL �����</param>
/// <returns>Wave ���� � ���� ������� ������</returns>
std::vector<BYTE> TSoundManager::CreateWaveFile(TIndexSound &is)
{
	int dataSize = is.Size - sizeof(SOUND_BLOCK);

	auto waveSound = std::vector<BYTE>(dataSize + sizeof(WaveHeader));
	auto waveHeader = (WaveHeader*) waveSound.data();

	strcpy(waveHeader->chunkId, "RIFF");
	strcpy(waveHeader->format, "WAVE");
	strcpy(waveHeader->subChunkId, "fmt ");
	strcpy(waveHeader->dataChunkId, "data");

	waveHeader->chunkSize = waveSound.size();
	waveHeader->subChunkSize = 16;
	waveHeader->audioFormat = 1;
	waveHeader->numChannels = 1;
	waveHeader->sampleRate = 22050;
	waveHeader->bitsPerSample = 16;
	waveHeader->bytesPerSecond = waveHeader->sampleRate * 
		(waveHeader->bitsPerSample/8) * waveHeader->numChannels;
	waveHeader->blockAlign = 2;	
	waveHeader->dataSize = dataSize;

	is.Timer = (DWORD)(dataSize / (((float)waveHeader->bytesPerSecond) / 1000.0f));

	auto sndDataPtr = (PBYTE)(is.Address + sizeof(SOUND_BLOCK));
	std::copy_n(sndDataPtr, dataSize, waveSound.begin() + sizeof(WaveHeader));	

	return waveSound;
}
//---------------------------------------------------------------------------
//Mix_Chunk *TSoundManager::LoadSoundEffect(TIndexSound &is)
//{
//	auto waveSound = this->CreateWaveFile(is);
//
//	SDL_RWops *rWops = SDL_RWFromMem(waveSound.data(), waveSound.size());
//	Mix_Chunk *mix = Mix_LoadWAV_RW(rWops, 1);
//
//	if (mix == NULL)
//		trace_printf("SDL sound error: %s\n", SDL_GetError());
//
//	return mix;
//}

HSTREAM TSoundManager::LoadSoundEffect(TIndexSound &is)
{
	auto waveSound = this->CreateWaveFile(is);
	BYTE* waveFileMem = new BYTE[waveSound.size()];	
	std::copy(waveSound.begin(), waveSound.end(), waveFileMem);
	
	auto hStream = BASS_StreamCreateFile(true, waveFileMem, 0, 
											waveSound.size(), 
											BASS_SAMPLE_FLOAT | BASS_SAMPLE_3D
											| BASS_SAMPLE_SOFTWARE);	

	if (hStream == 0)
	{
		trace_printf("BASS create stream error: %s\n", BASS_ErrorGetDescription());
		delete[] waveFileMem;
	}
	else
		this->streams[hStream] = waveFileMem;
	
	return hStream;
}

//---------------------------------------------------------------------------
//void TSoundManager::PlaySoundEffect(Mix_Chunk *mix, int volume)
//{
//	if (mix == NULL || GetForegroundWindow() != g_hWnd)
//		return;
//
//	Mix_VolumeChunk(mix, volume);
//
//	if (Mix_PlayChannel(-1, mix, 0) < 0)
//		trace_printf("Sound mix error: %s\n", Mix_GetError());
//}

void TSoundManager::PlaySoundEffect(HSTREAM hStream, int volume)
{
	if (volume > 255)
		volume = 255;
	else if (volume < 0)
		volume = 0;	

	if (hStream == 0 || GetForegroundWindow() != g_hWnd)
		return;

	BASS_ChannelSetAttribute(hStream, BASS_ATTRIB_VOL, ((float)volume) / 2.55f);

	if (!BASS_ChannelPlay(hStream, false))
		trace_printf("Bass sound play error: %s\n", BASS_ErrorGetDescription());

}
//---------------------------------------------------------------------------
/// <summary>������� ��������� ������ � ������������� ������.</summary>
/// <param name="hSteam">BASS stream ����������</param>
bool TSoundManager::FreeStream(HSTREAM hSteam)
{
	auto res = BASS_StreamFree(hSteam);
	auto waveFileIter = streams.find(hSteam);

	if (waveFileIter != streams.end()) {
		auto waveFile = waveFileIter->second;
		streams.erase(waveFileIter);
		delete[] waveFile;
	}

	return res;
}
//---------------------------------------------------------------------------
void TSoundManager::PlayMidi(int index)
{
	const char *musicSubCat[3] =
	{
		"",
		"\\512K",
		"\\4MB"
	};

	if (index >= 0 && index < MUSIC_COUNT)
	{
		StopMusic();

		if (!ConfigManager.Music || GetForegroundWindow() != g_hWnd)
			return;

		MCI_OPEN_PARMS openParm = {0};
		openParm.dwCallback = (DWORD)g_hWnd;
		openParm.lpstrDeviceType = L"sequencer";
		DWORD flags = MCI_OPEN_ELEMENT;

		char musicPath[100] = {0};
		sprintf(musicPath, "music%s\\%s", musicSubCat[1], m_MusicName[index]);

		wstring midiName = ToWString(musicPath);
		openParm.lpstrElementName = midiName.c_str();

		MCIERROR error = mciSendCommand(0, MCI_OPEN, flags,(DWORD)&openParm);

		if (!error)
		{
			m_Music = openParm.wDeviceID;

			SetMusicVolume(ConfigManager.MusicVolume);

			MCI_PLAY_PARMS playParm = {0};
			playParm.dwCallback = (DWORD)g_hWnd;
			playParm.dwFrom = 0;

			error = mciSendCommand(m_Music, MCI_PLAY, MCI_FROM | MCI_NOTIFY, (DWORD)&playParm);

			if (error)
				TraceMusicError(error);
		}
		else
			TraceMusicError(error);
	}
	else
		trace_printf("Music ID is out of renge: %i\n",index);
}
//---------------------------------------------------------------------------
void TSoundManager::StopMusic()
{
	if (m_Music != 0)
	{
		MCI_GENERIC_PARMS mciGen;
		DWORD error = mciSendCommand(m_Music, MCI_STOP, MCI_WAIT, (DWORD)(LPMCI_GENERIC_PARMS)&mciGen);

		TraceMusicError(error);

		m_Music = 0;
	}
}
//---------------------------------------------------------------------------
void TSoundManager::SetMusicVolume(int volume)
{
	if (m_Music != 0)
	{
	}
}
//---------------------------------------------------------------------------
void TSoundManager::TraceMusicError(DWORD error)
{
	if (error)
	{
		wchar_t szBuf[MAXERRORLENGTH];

		if (mciGetErrorString(error, szBuf, MAXERRORLENGTH))
			trace_printf("Midi error: %s\n", ToString(szBuf).c_str());
		else
			trace_printf("Midi error: #%i\n", error);
	}
}
//---------------------------------------------------------------------------
const char *TSoundManager::m_MusicName[MUSIC_COUNT] =
{
	"oldult01.mid",
	"create1.mid",
	"dragflit.mid",
	"oldult02.mid",
	"oldult03.mid",
	"oldult04.mid",
	"oldult05.mid",
	"oldult06.mid",
	"stones2.mid",
	"britain1.mid",
	"britain2.mid",
	"bucsden.mid",
	"jhelom.mid",
	"lbcastle.mid",
	"linelle.mid",
	"magincia.mid",
	"minoc.mid",
	"ocllo.mid",
	"samlethe.mid",
	"serpents.mid",
	"skarabra.mid",
	"trinsic.mid",
	"vesper.mid",
	"wind.mid",
	"yew.mid",
	"cave01.mid",
	"dungeon9.mid",
	"forest_a.mid",
	"intown01.mid",
	"jungle_a.mid",
	"mountn_a.mid",
	"plains_a.mid",
	"sailing.mid",
	"swamp_a.mid",
	"tavern01.mid",
	"tavern02.mid",
	"tavern03.mid",
	"tavern04.mid",
	"combat1.mid",
	"combat2.mid",
	"combat3.mid",
	"approach.mid",
	"death.mid",
	"victory.mid",
	"btcastle.mid",
	"nujelm.mid",
	"dungeon2.mid",
	"cove.mid",
	"moonglow.mid",
	"oldult02.mid",
	"serpents.mid",
	"oldult04.mid",
	"dragflit.mid",
	"create1.mid",
	"approach.mid",
	"combat3.mid",
	"jungle_a.mid"
};
//---------------------------------------------------------------------------