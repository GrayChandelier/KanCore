#pragma once
#include "Details.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <AL/al.h>
#include <AL/alc.h>

namespace KanCore::Audio
{

    struct SpatialState
    {
        Vec3f position;
        Vec3f velocity;
        Vec3f direction;
    };

    namespace AudioDetails
    {
        class SoundBuffer;
        using SoundBufferPtr = std::shared_ptr<SoundBuffer>;

        class IAudioDevicePrivate
        {
        public:
            virtual ALCdevice* getDevice() noexcept = 0;
        };

        class AudioDevice : public IAudioDevicePrivate
        {
        private:
            ALCdevice* device = nullptr;
            void initDevice(const char* deviceName);
        public:
            AudioDevice();
            explicit AudioDevice(const std::string& deviceName);
            AudioDevice(AudioDevice&) = delete;
            AudioDevice(AudioDevice&& other) noexcept;
            ~AudioDevice() noexcept;

            ALCdevice* getDevice() noexcept override;
        };

        class AudioSettings
        {
        private:
            ALCcontext* context = nullptr;
            AudioDevice device;
            float volume = 1.f;
            void setContext();
        public:
            AudioSettings()
            {
                setContext();
            }
            ~AudioSettings()
            {
                if (!context)
                    return;
                
                alcMakeContextCurrent(nullptr);
                alcDestroyContext(context);            
            }
            void setDevice(AudioDevice&& dev);
            void setOverallVolume(float value) noexcept;
            float getOverallVolume() const noexcept;

            void setGlobalDistanceModel(ALenum model);
            void setGlobalDopplerFactor(float factor);
            void setGlobalSpeedOfSound(float speed);
            void resetGlobalSettings();
        };

        class SoundLoader
        {
        public:
            SoundBufferPtr loadSound(ALenum format, const void* data, ALsizei size, ALsizei freq);
        };

        class SoundBuffer
        {
            friend class SoundLoader;
        private:
            ALuint bufferId = 0;
            SoundBuffer(ALenum fmt, const void* data, ALsizei sz, ALsizei fr);

        public:
            SoundBuffer(const SoundBuffer&) = delete;
            SoundBuffer& operator=(const SoundBuffer&) = delete;
            SoundBuffer(SoundBuffer&& old) noexcept;
            SoundBuffer& operator=(SoundBuffer&& old) noexcept;
            ~SoundBuffer() noexcept;

            ALuint getId() const noexcept { return bufferId; }
            bool isValid() const noexcept { return bufferId != 0 && alIsBuffer(bufferId); }
        };

        class SoundBufferStorage
        {
        private:
            using BufferKey = std::string;
            using BufferPtr = std::shared_ptr<SoundBuffer>;
            std::unordered_map<BufferKey, BufferPtr> buffers;
        public:
            BufferPtr getBuffer(const std::string& soundName) const;
            void saveBufferAs(const std::string& soundName, BufferPtr buffer);
            void eraseBuffer(const std::string& soundName);
        };

        class Listener
        {

        public:
            Listener& setPosition(Vec3f position);
            Listener& setDirection(Vec3f direction);
            Listener& setVelocity(Vec3f velocity);

            Vec3f getPosition();
            Vec3f getDirection();
            Vec3f getVelocity();
        };
    }

    using SoundBufferPtr = AudioDetails::SoundBufferPtr;

    class Sound
    {
    private:
        ALuint sourceId = 0;
        SoundBufferPtr buffer;
        void checkSource() const;
        bool isValid() const noexcept;
    public:
        explicit Sound(SoundBufferPtr buffer);

        Sound(const Sound&) = delete;
        Sound& operator=(const Sound&) = delete;
        Sound(Sound&&) noexcept;
        Sound& operator=(Sound&&) noexcept;
        ~Sound() noexcept;

        void stop();
        void pause();
        void resume();
        void play();

        Sound& setVolume(float volume);
        Sound& setPitch(float pitch);
        Sound& setDopplerVelocity(float velocity);

        Sound& rewind(std::chrono::duration<float> seconds);
        Sound& rewindTo(std::chrono::duration<float> seconds);
        Sound& looped(bool flag);

        //For 3D
        Sound& setSpatialParams(
            bool relative,
            float referenceDistance,
            float maxDistance,
            float rolloff
        );

        std::chrono::duration<float> getElapsedSeconds();
        bool isPlaying() const;
        bool isPaused() const;
        bool isStopped() const;

        Sound& setPosition(const Vec3f& pos);
        Vec3f getPosition() const;

    };

    class AudioContext
    {
    public:
        AudioDetails::AudioSettings settings;
        AudioDetails::SoundBufferStorage sounds;
        AudioDetails::Listener listener;
    };

    SoundBufferPtr loadFromFile(const std::string& path);
    SoundBufferPtr loadFromMemory(ALenum format, const void* data, ALsizei size, ALsizei freq);
}
