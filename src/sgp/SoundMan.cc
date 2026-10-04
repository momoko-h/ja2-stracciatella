/*********************************************************************************
* SGP Digital Sound Module
*
*		This module handles the playing of digital samples, preloaded or streamed.
*
* Derek Beland, May 28, 1997
*********************************************************************************/

#include "SoundMan.h"

#include "ContentManager.h"
#include "GameInstance.h"
#include "Logger.h"
#include "Random.h"
#include "SGPFile.h"
#include "Types.h"

#include <string_theory/format>
#include <algorithm>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <variant>
#include <vector>

// Miniaudio includes needs some defines

#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_GENERATION
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#include <miniaudio.h>

#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

static BOOLEAN fSoundSystemInit = FALSE; // Startup called

bool IsSoundEnabled()
{
	return fSoundSystemInit;
}

namespace SoundMan {
constexpr float SoundLibsConversionFactor{ 127.0f };
// Convert an integer in the range [0..127] as used by the Miles Sound System
// to a float in the range [0..1] as used by miniaudio.
constexpr float MSSVolumetoMA(UINT32 value)
{
	return float(std::min(value, 127U)) / SoundLibsConversionFactor;
}

// In MSS, a pan value of 0 shifts left, 63 and 64 do not shift and 127
// shifts right. In miniaudio, -1 shifts left and +1 shifts right.
constexpr float MSSPantoMA(UINT32 value)
{
	return MSSVolumetoMA(value);
//	return float(std::min(value, 127U)) * (2.0f / SoundLibsConversionFactor) - 1.0f;
}


static void Require(ma_result result, char const * functionName)
{
	if (result == MA_SUCCESS) return;

	throw std::runtime_error(ST::format("miniaudio call of {} failed: {}",
		functionName, ma_result_description(result)).c_str());
}


static UINT32 CreateUniqueID()
{
	static UINT32 currentID{ 0 };
	return ++currentID;
}


static ma_result MiniaudioReadProc(ma_decoder * pDecoder,
	void * pBufferOut, size_t bytesToRead, size_t * bytesRead)
{
	auto * file{ reinterpret_cast<SGPFile *>(pDecoder->pUserData) };

	try {
		*bytesRead = file->readAtMost(pBufferOut, bytesToRead);
	}
	catch (...) {
		return MA_IO_ERROR;
	}
	return MA_SUCCESS;
}


static ma_result MiniaudioSeekProc(ma_decoder * pDecoder,
	ma_int64 byteOffset, ma_seek_origin origin)
{
	auto * file{ reinterpret_cast<SGPFile *>(pDecoder->pUserData) };

	try
	{
		file->seek(static_cast<INT32>(byteOffset), [origin] {
			switch (origin)
			{
				default:
				case ma_seek_origin_current: return FILE_SEEK_FROM_CURRENT;
				case ma_seek_origin_start:   return FILE_SEEK_FROM_START;
				case ma_seek_origin_end:     return FILE_SEEK_FROM_END;
			}}());
	}
	catch (...)
	{
		return MA_BAD_SEEK;
	}
	return MA_SUCCESS;
}


struct SoundObject
{
	struct AudioFile
	{
		std::unique_ptr<SGPFile> sgpFile;
		ma_decoder decoder;

		AudioFile(char const * filename)
			: sgpFile{ GCM->openGameResForReading(filename) }
		{
			Require(ma_decoder_init(MiniaudioReadProc, MiniaudioSeekProc,
				sgpFile.get(), nullptr, &decoder), "ma_decoder_init");
		}

		~AudioFile()
		{
			ma_decoder_uninit(&decoder);
		}
	};

	struct AudioBuffer
	{
		std::vector<UINT8> rawPCMData;
		ma_audio_buffer audioBuffer;

		// The audio object takes ownership of the underlying memory.
		AudioBuffer(std::vector<UINT8> & sourceData)
			: rawPCMData{ std::move(sourceData) }
		{
		}

		~AudioBuffer()
		{
			ma_audio_buffer_uninit(&audioBuffer);
		}
	};

	ma_sound sound;
	std::variant<std::monostate, AudioFile, AudioBuffer> dataSource;

	void (*endCallback)(void *);
	void * callbackUserData;

	// The name that was passed to PlaySound(). Not really used but helps
	// during debugging;
	ST::string name;

	// # of times we still have to play this sound.
	UINT32 loops{ 1 };

	// Random sounds are not automatically removed when their loop count
	// drops to 0.
	bool isRandom{ false };

	~SoundObject()
	{
		ma_sound_uninit(&sound);
	}
};

std::map<SoundManagerID, SoundObject> Sounds;
ma_engine Engine;


static void Start(SoundObject & so, UINT32 volume, UINT32 pan)
{
	ma_sound_set_volume(&so.sound, MSSVolumetoMA(volume));
	ma_sound_set_pan(&so.sound, MSSPantoMA(pan));
	ma_sound_start(&so.sound);
}


static auto FromID(SoundManagerID id)
{
	auto pos{ Sounds.find(id) };
	return pos != Sounds.end() ? &pos->second : nullptr;
}


static void InitDataSource(ma_data_source * source, SoundObject & soundObject)
{
	Require(ma_sound_init_from_data_source(&Engine, source,
		MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC,
		nullptr, &soundObject.sound), "ma_sound_init_from_data_source");
}


static auto GetSoundObject()
{
	auto [ pos, created ] { Sounds.try_emplace(CreateUniqueID()) };
	if (created) return pos;

	throw std::logic_error("Could not create a new SoundObject");
}


// Create a new sound object for a SGPFile.
static auto GetSoundObject(char const * filename)
{
	auto pos{ GetSoundObject() };
	SoundObject & soundObject{ pos->second };

	auto & audioFile{ soundObject.dataSource.emplace<SoundObject::AudioFile>(filename) };

	InitDataSource(&audioFile.decoder, soundObject);
	return pos;
}


// Create a new sound object for a memory buffer of raw PCM data.
static auto GetSoundObject(std::vector<UINT8> & rawData, ma_format format,
	UINT32 channels, UINT32 rate)
{
	auto pos{ GetSoundObject() };
	SoundObject & soundObject{ pos->second };
	ma_uint64 frames{ (rawData.size() / channels) / (format == ma_format_s16 ? 2 : 1) };

	auto & audioBuffer{ soundObject.dataSource.emplace<SoundObject::AudioBuffer>(rawData) };

	auto config { ma_audio_buffer_config_init(format,
		channels, frames, audioBuffer.rawPCMData.data(), nullptr) };
	config.sampleRate = rate;

	Require(ma_audio_buffer_init(&config, &audioBuffer.audioBuffer), "ma_audio_buffer_init");

	InitDataSource(&audioBuffer.audioBuffer, soundObject);
	return pos;
}


/* Searches out a sound instance referred to by its ID number.
 *
 * Returns: If the instance was found, the pointer to the channel.  NULL
 *          otherwise. */
static ma_sound * SoundGetByID(SoundManagerID id)
{
	if (!fSoundSystemInit) return nullptr;

	auto soundObject = FromID(id);
	return soundObject ? &soundObject->sound : nullptr;
}


std::optional<decltype(GetSoundObject())> SoundPlay(const char * pFilename,
	UINT32 loop, void (*end_callback)(void *), void * data)
{
	if (!fSoundSystemInit) return std::nullopt;

	auto emplaceResult{ GetSoundObject(pFilename) };
	auto & soundObject { emplaceResult->second };

	soundObject.endCallback = end_callback;
	soundObject.callbackUserData = data;
	soundObject.name = pFilename;
	soundObject.loops = loop;

	return emplaceResult;
}
}

using namespace SoundMan;


void InitializeSoundManager(bool noSound)
{
	if (fSoundSystemInit || noSound) return;

	auto cfg { ma_engine_config_init() };
	Require(ma_engine_init(&cfg, &Engine), "ma_engine_config_init");

	fSoundSystemInit = true;
}


void ShutdownSoundManager(void)
{
	SoundStopAll();
	ma_engine_uninit(&Engine);
	fSoundSystemInit = false;
}


SoundManagerID SoundPlay(const char * pFilename, UINT32 volume, UINT32 pan,
	UINT32 loop, void (*end_callback)(void *), void * data)
{
	auto optionalEmplaceResult{ SoundPlay(pFilename, loop, end_callback, data) };
	if (!optionalEmplaceResult) return SOUND_ERROR;

	auto && [ id, soundObject] { **optionalEmplaceResult };

	Start(soundObject, volume, pan);
	return id;
}

/* Play a sound sample from a Smacker Flick
 *
 * Allocates space for the sound sample within the sound system
 */
SoundManagerID SoundPlayRawPCMData(char const * name, UINT8 channels,
	UINT8 depth, UINT32 rate, std::vector<UINT8> & buf, UINT32 volume, UINT32 pan)
{
	SLOGD("SoundPlayRawPCMData {}", name);

	if (buf.empty()) return SOUND_ERROR;

	//Originaly Sound Blaster could only play mono unsigned 8-bit PCM data.
	//Later it became capable of playing 16-bit audio data, but needed to be signed and LSB.
	//They were the de facto standard so I'm assuming smacker uses the same.
	ma_format format;
	if (depth == 8) format = ma_format_u8;
	else if (depth == 16) format = ma_format_s16;
	else return SOUND_ERROR;

	if (format == ma_format_s16) {
		// We expect the Endianess for the Smacker buffer to be little endian, but ma_format_s16 is native endian, so we need to do some conversion
		convertLittleEndianBufferToNativeEndianU16(buf.data(), static_cast<uint32_t>(buf.size()));
	}

	auto && [ id, soundObject] { *GetSoundObject(buf, format, channels, rate) };

	soundObject.name = name;
	Start(soundObject, volume, pan);
	return id;
}


bool SoundIsPlaying(SoundManagerID id)
{
	ma_sound * const sound { SoundGetByID(id) };
	return sound && ma_sound_is_playing(sound);
}


bool SoundStop(SoundManagerID id)
{
	return Sounds.erase(id) != 0;
}


void SoundStopAll(void)
{
	Sounds.clear();
}


bool SoundSetVolume(SoundManagerID id, UINT32 uiVolume)
{
	ma_sound * const sound { SoundGetByID(id) };
	if (!sound) return FALSE;

	ma_sound_set_volume(sound, MSSVolumetoMA(uiVolume));
	return TRUE;
}


bool SoundSetPan(SoundManagerID id, UINT32 uiPan)
{
	ma_sound * const sound { SoundGetByID(id) };
	if (!sound) return FALSE;

	ma_sound_set_pan(sound, MSSPantoMA(uiPan));
	return TRUE;
}


UINT32 SoundGetVolume(SoundManagerID id)
{
	ma_sound * const sound { SoundGetByID(id) };
	if (!sound) return SOUND_ERROR;

	return static_cast<UINT32>(ma_sound_get_volume(sound) * SoundLibsConversionFactor);
}


namespace SoundMan
{
auto RndRange(UINT32 min, UINT32 max)
{
	return std::uniform_int_distribution{ min, max }(gRandomEngine);
};

struct RandomSound
{
	RandomSoundID  ownID;
	SoundManagerID soundID;
	UINT32 timeMin, timeMax;
	UINT32 volMin, volMax;
	UINT32 panMin, panMax;

	void Queue(SoundObject & soundObject) const
	{
		ma_sound * sound{ &soundObject.sound };
		UINT32 delay{ RndRange(timeMin, timeMax) };
		ma_sound_set_start_time_in_milliseconds(sound,
			ma_engine_get_time_in_milliseconds(&Engine) + delay);
		Start(soundObject, RndRange(volMin, volMax), RndRange(panMin, panMax));

		SLOGD("Queued random sound '{}' (ID {}), starting in {} ms",
			soundObject.name, ownID, delay);
	}
};

std::vector<RandomSound> RandomSounds;
}


RandomSoundID SoundPlayRandom(const char* pFilename,
	UINT32 time_min, UINT32 time_max, UINT32 vol_min, UINT32 vol_max,
	UINT32 pan_min, UINT32 pan_max)
{
	if (!fSoundSystemInit) return NO_RANDOM_SAMPLE;

	auto & randomSound{ RandomSounds.emplace_back(CreateUniqueID(), NO_SAMPLE,
		time_min, time_max, vol_min, vol_max, pan_min, pan_max) };

	auto optionalEmplaceResult{ SoundPlay(pFilename, 1, nullptr, nullptr) };
	if (!optionalEmplaceResult)
	{
		RandomSounds.pop_back();
		return NO_RANDOM_SAMPLE;
	}

	auto & soundObject{ (**optionalEmplaceResult).second };
	randomSound.soundID = (**optionalEmplaceResult).first;
	soundObject.isRandom = true;

	randomSound.Queue(soundObject);
	return randomSound.ownID;
}


void SoundStopRandom(RandomSoundID rid)
{
	for (auto it{ RandomSounds.begin() }; it != RandomSounds.end(); ++it)
	{
		if (it->ownID == rid)
		{
			SoundStop(it->soundID);
			RandomSounds.erase(it);
			return;
		}
	}
}


void SoundStopAllRandom()
{
	while (!RandomSounds.empty())
	{
		SoundStopRandom(RandomSounds.back().ownID);
	}
}


UINT32 SoundGetPosition(SoundManagerID id)
{
	ma_sound * const sound { SoundGetByID(id) };
	if (!sound) return 0;

	float cursor;
	return ma_sound_get_cursor_in_seconds(sound, &cursor) == MA_SUCCESS
		? static_cast<UINT32>(cursor * 1000.0f)
		: 0;
}


void SoundServiceStreams()
{
	if (!fSoundSystemInit) return;

	// Garbage collect all sounds that have already finished playing.
	for (auto it{ Sounds.begin() }; it != Sounds.end(); )
	{
		auto & soundObject{ it-> second };

		if (!soundObject.isRandom)
		{
			if (ma_sound_at_end(&soundObject.sound))
			{
				if (--soundObject.loops == 0)
				{
					if (soundObject.endCallback)
					{
						soundObject.endCallback(soundObject.callbackUserData);
					}

					it = Sounds.erase(it);
					continue;
				}
				else
				{
					ma_sound_start(&soundObject.sound);
				}
			}
		}

		++it;
	}

	for (auto const& random : RandomSounds)
	{
		auto * soundObject{ FromID(random.soundID) };
		if (!soundObject) continue;

		if (auto * sound{ &soundObject->sound }; ma_sound_at_end(sound))
		{
			// Finished playing, restart (miniaudio handles the wait time).
			random.Queue(*soundObject);
		}
	}
}
