#include "../include/Audio.hpp"
#include <sndfile.h>
#include <utility>

// ================= AudioDevice =================
namespace KanCore::Audio::AudioDetails 
{

    void AudioDevice::initDevice(const char* deviceName = nullptr)
    {
        device = alcOpenDevice(deviceName);
        if (!device) {
            auto devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
            printf("Available devices: %s\n", devices ? devices : "none");
            throw std::runtime_error("Failed to open audio device");
        }
    }

    AudioDevice::AudioDevice()
    {
        initDevice(nullptr);
    }

    AudioDevice::AudioDevice(const std::string& deviceName)
    {
        initDevice(deviceName.c_str());
    }

    AudioDevice::AudioDevice(AudioDevice&& other) noexcept
        : device(std::exchange(other.device, nullptr))
    {
        printf("device moved\n");
    }

    AudioDevice::~AudioDevice() noexcept
    {
        if (device) alcCloseDevice(device);
    }

    ALCdevice* AudioDevice::getDevice() noexcept
    {
        return device;
    }

} // namespace AudioDetails


// ================= AudioSettings =================
namespace KanCore::Audio::AudioDetails 
{

    void AudioSettings::setContext()
    {
        if (!device.getDevice()) throw std::runtime_error("Audio device is not set");

        //destory old context
        if (context)
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
            context = nullptr;
        }

        context = alcCreateContext(device.getDevice(), nullptr);
        if (!context) throw std::runtime_error("Failed to create OpenAL context");

        if (!alcMakeContextCurrent(context))
        {
            alcDestroyContext(context);
            context = nullptr;
            throw std::runtime_error("Failed to make OpenAL context current");
        }



    }

    void AudioSettings::setDevice(AudioDevice&& dev)
    {

        setContext();
    }

    void AudioSettings::setOverallVolume(float value) noexcept
    {
        volume = value;
        alListenerf(AL_GAIN, volume);
    }

    float AudioSettings::getOverallVolume() const noexcept
    {
        return volume;
    }

    SoundBuffer::SoundBuffer(ALenum fmt, const void* data, ALsizei sz, ALsizei fr)
    {
        alGenBuffers(1, &bufferId);
        if (ALenum err = alGetError(); err != AL_NO_ERROR)
        {
            throw std::runtime_error("Failed to generate OpenAL buffer");
        }

        alBufferData(bufferId, fmt, data, sz, fr);
        if (ALenum err = alGetError(); err != AL_NO_ERROR) {
            alDeleteBuffers(1, &bufferId);
            throw std::runtime_error("Failed to fill OpenAL buffer");
        }
    }

    SoundBuffer::SoundBuffer(SoundBuffer&& old) noexcept
        : bufferId(std::exchange(old.bufferId, 0)) {
    }

    SoundBuffer& SoundBuffer::operator=(SoundBuffer&& old) noexcept
    {
        if (this != &old)
        {
            if (bufferId && alIsBuffer(bufferId))
                alDeleteBuffers(1, &bufferId);
            bufferId = std::exchange(old.bufferId, 0);
        }
        return *this;
    }

    SoundBuffer::~SoundBuffer() noexcept
    {
        if (bufferId && alIsBuffer(bufferId))
            alDeleteBuffers(1, &bufferId);
    }

    SoundBufferStorage::BufferPtr SoundBufferStorage::getBuffer(const std::string& soundName) const
    {
        auto it = buffers.find(soundName);
        if (it == buffers.end())
            throw std::runtime_error("Failed to get Audio::BufferPtr with name '" + soundName + "'");

        return it->second;
    }
    void SoundBufferStorage::eraseBuffer(const std::string& soundName)
    {
        buffers.erase(soundName);
    }

    void SoundBufferStorage::saveBufferAs(const std::string& soundName, BufferPtr buffer)
    {
        buffers[soundName] = buffer;
    }

    Listener& Listener::setPosition(Vec3f position)
    {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
        return *this;
    }
    Listener& Listener::setDirection(Vec3f direction)
    {
        float orient[6] = { direction.x, direction.y, direction.z, 0.f, 1.f, 0.f };
        alListenerfv(AL_ORIENTATION, orient);
        return *this;
    }
    Listener& Listener::setVelocity(Vec3f velocity)
    {
        alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
        return *this;
    }

    Vec3f Listener::getPosition()
    {
        Vec3f v;
        alGetListener3f(AL_POSITION, &v.x, &v.y, &v.z);
        return v;
    }
    Vec3f Listener::getDirection()
    {
        Vec3f v;
        float orient[6];
        alGetListenerfv(AL_ORIENTATION, orient);

        v.x = orient[0];
        v.y = orient[1];
        v.z = orient[2];

        return v;
    }
    Vec3f Listener::getVelocity()
    {
        Vec3f v;
        alGetListener3f(AL_VELOCITY, &v.x, &v.y, &v.z);
        return v;
    }

    SoundBufferPtr SoundLoader::loadSound(ALenum format, const void* data, ALsizei size, ALsizei freq)
    {
        return std::shared_ptr<SoundBuffer>(new SoundBuffer(format, data, size, freq));
    }

    void AudioSettings::setGlobalDistanceModel(ALenum model)
    {
        alDistanceModel(model);
        ALenum err = alGetError();
        if (err != AL_NO_ERROR)
            throw std::runtime_error("Failed to set distance model: " + std::to_string(err));
    }

    void AudioSettings::setGlobalDopplerFactor(float factor)
    {
        alDopplerFactor(factor);
        ALenum err = alGetError();
        if (err != AL_NO_ERROR)
            throw std::runtime_error("Failed to set doppler factor: " + std::to_string(err));
    }

    void AudioSettings::setGlobalSpeedOfSound(float speed)
    {
        alSpeedOfSound(speed);
        ALenum err = alGetError();
        if (err != AL_NO_ERROR)
            throw std::runtime_error("Failed to set speed of sound: " + std::to_string(err));
    }

    void AudioSettings::resetGlobalSettings()
    {
        setGlobalDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
        setGlobalDopplerFactor(1.0f);
        setGlobalSpeedOfSound(343.3f); 
    }
} 


namespace KanCore::Audio
{
    bool Sound::isValid() const noexcept 
    { 
        return sourceId != 0 && alIsSource(sourceId); 
    }

    void Sound::checkSource() const
    {
        if (!isValid())
            throw std::runtime_error("Invalid OpenAL sound source");
    }

    Sound::Sound(SoundBufferPtr buf) : buffer(std::move(buf))
    {
        if (!buffer)
            throw std::invalid_argument("Invalid sound buffer");

        alGenSources(1, &sourceId);
        if (ALenum err = alGetError(); err != AL_NO_ERROR) {
            throw std::runtime_error("Failed to generate OpenAL source");
        }

        alSourcei(sourceId, AL_BUFFER, static_cast<ALint>(buffer->getId()));
        if (ALenum err = alGetError(); err != AL_NO_ERROR) {
            alDeleteSources(1, &sourceId);
            sourceId = 0;
            throw std::runtime_error("Failed to attach buffer to source");
        }

        setSpatialParams(false, 1.f, 100.f, 1.f);
    }

    Sound::Sound(Sound&& other) noexcept
        : sourceId(std::exchange(other.sourceId, 0)), buffer(std::move(other.buffer)) 
    {}

    Sound& Sound::operator=(Sound&& other) noexcept
    {
        if (this != &other) 
        {
            if (sourceId && alIsSource(sourceId)) alDeleteSources(1, &sourceId);
            sourceId = std::exchange(other.sourceId, 0);
            buffer = std::move(other.buffer);
        }
        return *this;
    }

    Sound::~Sound() noexcept
    {
        if (sourceId && alIsSource(sourceId)) 
            alDeleteSources(1, &sourceId);
    }

    void Sound::stop() 
    { 
        checkSource(); 
        alSourceStop(sourceId); 
    }
    void Sound::pause()
    {
        checkSource();
        alSourcePause(sourceId);
    }
    void Sound::resume() 
    { 
        checkSource(); 
        alSourcePlay(sourceId); 
    }
    void Sound::play() 
    { 
        checkSource(); 
        alSourcePlay(sourceId); 
        ALenum err = alGetError();
        if (err != AL_NO_ERROR)
            throw std::runtime_error("OpenAL play failed: " + std::to_string(err));
    }

    Sound& Sound::setSpatialParams(
        bool relative,
        float referenceDistance,
        float maxDistance,
        float rolloff)
    {
        checkSource();

        alSourcei(sourceId, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
        alSourcef(sourceId, AL_REFERENCE_DISTANCE, referenceDistance);
        alSourcef(sourceId, AL_MAX_DISTANCE, maxDistance);
        alSourcef(sourceId, AL_ROLLOFF_FACTOR, rolloff);

        return *this;
    }
    Sound& Sound::setPosition(const Vec3f& pos)
    {
        checkSource();
        alSource3f(sourceId, AL_POSITION, pos.x, pos.y, pos.z);
        return *this;
    }

    Vec3f Sound::getPosition() const
    {
        checkSource();
        Vec3f v;
        alGetSource3f(sourceId, AL_POSITION, &v.x, &v.y, &v.z);
        return v;
    }

    Sound& Sound::setVolume(float v) 
    { 
        checkSource(); 
        alSourcef(sourceId, AL_GAIN, v); 
        return *this; 
    }
    Sound& Sound::setPitch(float p) 
    { 
        checkSource(); 
        alSourcef(sourceId, AL_PITCH, p);
        return *this; 
    }
    Sound& Sound::setDopplerVelocity(float v) 
    { 
        checkSource(); 
        alSourcef(sourceId, AL_DOPPLER_FACTOR, v); 
        return *this; 
    }
    Sound& Sound::rewind(std::chrono::duration<float> sec) 
    { 
        checkSource();
        ALfloat current; 
        alGetSourcef(sourceId, AL_SEC_OFFSET, &current); 
        alSourcef(sourceId, AL_SEC_OFFSET, current + sec.count()); 
        return *this; 
    }
    Sound& Sound::rewindTo(std::chrono::duration<float> sec) 
    { 
        checkSource();
        alSourcef(sourceId, AL_SEC_OFFSET, sec.count());
        return *this;
    }
    Sound& Sound::looped(bool flag) 
    { 
        checkSource(); 
        alSourcei(sourceId, AL_LOOPING, flag ? AL_TRUE : AL_FALSE); 
        return *this;
    }
    std::chrono::duration<float> Sound::getElapsedSeconds() 
    {
        checkSource(); 
        ALfloat sec;
        alGetSourcef(sourceId, AL_SEC_OFFSET, &sec); 
        return std::chrono::duration<float>(sec); 
    }
    bool Sound::isPlaying() const 
    { 
        checkSource(); ALint state; 
        alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING; 
    }
    bool Sound::isPaused() const 
    { 
        checkSource(); 
        ALint state;
        alGetSourcei(sourceId, AL_SOURCE_STATE, &state); 
        return state == AL_PAUSED;
    }
    bool Sound::isStopped() const 
    {
        checkSource();
        ALint state; 
        alGetSourcei(sourceId, AL_SOURCE_STATE, &state); 
        return state == AL_STOPPED; 
    }

    
    SoundBufferPtr loadFromFile(const std::string& path) 
    { 
        SF_INFO sfInfo{}; 
        SNDFILE* file = sf_open(path.c_str(), SFM_READ, &sfInfo); 

        if (!file)
            throw std::runtime_error("Failed to open audio file: " + path); 

        std::vector<short> data(sfInfo.frames * sfInfo.channels); 

        sf_readf_short(file, data.data(), sfInfo.frames); 

        sf_close(file); 
        ALenum format = AL_NONE; 

        if (sfInfo.channels == 1)
            format = AL_FORMAT_MONO16; 
        else if (sfInfo.channels == 2) 
            format = AL_FORMAT_STEREO16; 
        else 
            throw std::runtime_error("Unsupported channel count"); 


        AudioDetails::SoundLoader loader; 
        return loader.loadSound(format, data.data(), static_cast<ALsizei>(data.size() * sizeof(short)), sfInfo.samplerate); 
    }
    
    SoundBufferPtr loadFromMemory(ALenum format, const void* data, ALsizei size, ALsizei freq)
    {
        AudioDetails::SoundLoader loader;
        return loader.loadSound(format, data, size, freq);
    }
}
