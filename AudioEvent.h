#pragma once

class AudioEvent
{
public:
	//音源のファイル名
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
	unsigned int eventID_;//イベント識別用のID
	class AudioSystem* audio_;


	//音量(倍率)
	float volume_;

};

