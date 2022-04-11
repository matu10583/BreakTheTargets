#pragma once

class AudioEvent
{
public:
	//�����̃t�@�C����
	AudioEvent(const char* fileName,
		const unsigned int aeFlag);
	~AudioEvent();
	void Play();
	void Stop();
	void Pause();
	unsigned int GetID()const {
		return eventID_;
	}
	float GetVolume()const { return volume_; }
	void SetVolume(float vol);
	void SetLoop(bool loop);
	void UpdateEmitter(float deltaTime, class Object* obj);

	bool IsPlaying()const;

private:
	unsigned int eventID_;//�C�x���g���ʗp��ID
	class AudioSystem* audio_;


	//����(�{��)
	float volume_;

};

