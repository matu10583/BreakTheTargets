#pragma once
#include <stdint.h>
#include<windows.h>
#include <memory>
#include <mmreg.h>

//チャンクデータ構造
struct Chunk
{
	uint32_t id;//チャンクのID
	int32_t size;//チャンクサイズ
};

//FMTチャンク
struct FormatChunk
{
	Chunk chunk;//fmt 
	WAVEFORMATEXTENSIBLE info;//波形フォーマット
};

//waveファイルの情報
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
	//最後まで到達したか範囲外だとfalse,それ以外はtrue.readは実際に読んだバイト数
	bool GetWAVData(
		BYTE* buff, size_t sizeInByte, DWORD cursor, DWORD* read=nullptr)const;

private:

	WaveInfo info_;
	std::unique_ptr<uint8_t[]> data_;
};

