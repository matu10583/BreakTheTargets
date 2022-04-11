#include "WaveFile.h"
#include <cassert>
#include <fstream>

const uint32_t FOURCC_RIFF_TAG = 'FFIR';
const uint32_t FOURCC_FORMAT_TAG = ' tmf';
const uint32_t FOURCC_DATA_TAG = 'atad';
const uint32_t FOURCC_WAVE_FILE_TAG = 'EVAW';
const uint32_t FOURCC_XWMA_FILE_TAG = 'AMWX';
const uint32_t FOURCC_DLS_SAMPLE = 'pmsw';
const uint32_t FOURCC_MIDI_SAMPLE = 'lpms';
const uint32_t FOURCC_XWMA_DPDS = 'sdpd';
const uint32_t FOURCC_XMA_SEEK = 'kees';

WaveFile::WaveFile():
	info_({ 0 })
{
}

bool 
WaveFile::ReadFile(const char* fileName) {
	FILE* wav = nullptr;
	//open file
	fopen_s(&wav, fileName, "rb");
	if (wav == nullptr) {
		assert(0);
		fclose(wav);
		return false;
	}
	//Riff�w�b�_�[
	struct RiffHeader
	{
		Chunk chunk;//Riff
		uint32_t type;//Wave
	};
	RiffHeader riff;

	//Riff�w�b�_�[
	fread(&riff, sizeof(RiffHeader), 1, wav);
	if (riff.type != FOURCC_XWMA_FILE_TAG &&
		riff.type != FOURCC_WAVE_FILE_TAG) {
		//��Ή�
		return false;
	}

	//Format�`�����N
	Chunk chk;
	fread(&chk, sizeof(Chunk), 1, wav);
	if (chk.id != FOURCC_FORMAT_TAG) {
		return false;
	}
	info_.fmt = { 0 };

	//�����Ɠǂ߂�\���̂ɂȂ�܂ŌJ��Ԃ�
	fread(&info_.fmt, sizeof(WAVEFORMAT), 1, wav);
	bool dpds = false;//���ꂼ��̏�񂪂��邩
	bool seek = false;
	switch (info_.fmt.Format.wFormatTag)
	{
	case WAVE_FORMAT_PCM://WAVEFORMATPCM
		if (chk.size < 18) {
			fread(&info_.fmt.Format.wBitsPerSample, sizeof(WORD), 1, wav);
		}
		else {
			fread(&info_.fmt.Format.wBitsPerSample, sizeof(WORD), 2, wav);
		}
		break;
	case WAVE_FORMAT_IEEE_FLOAT:
		fread(&info_.fmt.Format.wBitsPerSample, sizeof(WORD), 1, wav);
		//�ǂ߂Ă�
		break;
	default://�ǂ߂ĂȂ�
		long long size = static_cast<long long>(sizeof(WAVEFORMAT));
		fseek(wav, -size, SEEK_CUR);
		fread(&info_.fmt, sizeof(WAVEFORMATEX), 1, wav);
		
		switch (info_.fmt.Format.wFormatTag)
		{
		case WAVE_FORMAT_WMAUDIO2:
		case WAVE_FORMAT_WMAUDIO3:
			dpds = true;
			//�ǂ߂Ă�
			break;
		case WAVE_FORMAT_ADPCM://ADPCMWAVEFORMAT
			size = static_cast<long long>(sizeof(WAVEFORMATEX));
			fseek(wav, -size, SEEK_CUR);
			fseek(wav, sizeof(ADPCMWAVEFORMAT), SEEK_CUR);
			
			break;
		case 0x166://WAVE_FORMAT_ZMA2
			seek = true;
			break;
		default://�ǂ߂ĂȂ�
			size = static_cast<long long>(sizeof(WAVEFORMATEX));
			fseek(wav, -size, SEEK_CUR);
			fread(&info_.fmt, sizeof(WAVEFORMATEXTENSIBLE), 1, wav);
			switch (info_.fmt.SubFormat.Data1)
			{
			case WAVE_FORMAT_PCM:
			case WAVE_FORMAT_IEEE_FLOAT:
				break;
			case WAVE_FORMAT_WMAUDIO2:
			case WAVE_FORMAT_WMAUDIO3:
				dpds = true;
				break;
			default:
				//��Ή�
				fclose(wav);
				return false;
				break;
			}

			break;
		}

		break;
	}

	//Data�`�����N���݂���܂Ŕ�΂�
	while (
		fread(&chk, sizeof(Chunk), 1, wav) == 1
		)
	{
		char* c = reinterpret_cast<char*>(&chk.id);
		char str[4];
		std::copy(c, c + 4, str);
		if (chk.id == FOURCC_DATA_TAG) {
			break;
		}
		fseek(wav, chk.size, SEEK_CUR);
	}
	info_.data = chk;

	//�f�[�^�T�C�Y���̗̈�m��
	data_.reset(new uint8_t[info_.data.size]);
	fread(data_.get(), info_.data.size, 1, wav);

	fclose(wav);

	return true;
}



bool
WaveFile::GetWAVData(
	BYTE* buff, size_t sizeInByte, DWORD cursor, DWORD* read)const {
	DWORD dsize = static_cast<DWORD>(info_.data.size);
	if (cursor > dsize) {
		assert(0 && "�͈͊O�ł�");
		*read = 0;
		return false;
	}
	auto rest = dsize - cursor;//�c��T�C�Y
	*read = sizeInByte;
	if (sizeInByte >= rest) {
		*read = rest;
	}

	//�ǂݍ���
	std::memcpy(buff, &data_[cursor], *read);
	return true;
}