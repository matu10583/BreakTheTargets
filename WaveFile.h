#pragma once
#include <stdint.h>
#include<windows.h>
#include <memory>
#include <mmreg.h>

//�`�����N�f�[�^�\��
struct Chunk
{
	uint32_t id;//�`�����N��ID
	int32_t size;//�`�����N�T�C�Y
};

//FMT�`�����N
struct FormatChunk
{
	Chunk chunk;//fmt 
	WAVEFORMATEXTENSIBLE info;//�g�`�t�H�[�}�b�g
};

//wave�t�@�C���̏��
struct WaveInfo {
	WAVEFORMATEXTENSIBLE fmt;
	Chunk data;
	uint32_t loopStart;
	uint32_t loopLength;
};

class WaveFile
{
public:
	WaveFile();
	bool ReadFile(const char* fileName);
	const WaveInfo& Info()const { return info_; }
	size_t GetSize()const { 
		auto ret = static_cast<size_t>(info_.data.size);
		return ret; }
	BYTE* GetStartAudio()const { return (BYTE*)data_.get(); }
	UINT32 GetLastSampleNum()const {
		return info_.data.size / info_.fmt.Format.nBlockAlign;
	}
	//�Ō�܂œ��B�������͈͊O����false,����ȊO��true.read�͎��ۂɓǂ񂾃o�C�g��
	bool GetWAVData(
		BYTE* buff, size_t sizeInByte, DWORD cursor, DWORD* read=nullptr)const;

private:

	WaveInfo info_;
	std::unique_ptr<uint8_t[]> data_;
};

