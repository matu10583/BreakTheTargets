#include "TextSpriteComponent.h"
#include "Sprite.h"
#include "Dx12Wrapper.h"
#include "Object.h"
#include "Game.h"

TextSpriteComponent::TextSpriteComponent(
	Object* owner, const char* fileName, int updateOrder) :
SpriteComponent(owner, fileName)
{
	auto charScale = Vector2(1.0f / X_CHAR_NUM, 1.0f / Y_CHAR_NUM);
	SetClipSize(charScale);
	charSize_ = sprite_->GetSize();
	charSize_.x /= X_CHAR_NUM;
	charSize_.y /= Y_CHAR_NUM;
}

void
TextSpriteComponent::Update(float deltaTime) {
	//�ړ��A��]���̍X�V

	//���̂܂܂��ƃt�H���g�摜�̃X�P�[���ɂȂ��Ă���̂�
	//�땶��������̃X�P�[���ɒ���
	auto scmat = XMMatrixScaling(
		scale_.x / X_CHAR_NUM,
		scale_.y / Y_CHAR_NUM, 1.0f);

	mappedTransMatrix_->world =
		scmat *
		XMMatrixRotationZ(rot_) *
		XMMatrixTranslation(pos_.x, pos_.y, pos_.z);
	mappedTransMatrix_->pivot = pivot_;
	mappedTransMatrix_->clipSize = clipSize_;
	mappedTransMatrix_->offset = offset_;

	//�\�����鎚�̃T�C�Y�����߂�
	auto s = sprite_->GetSize();
	auto sps = Vector2(XMVector2Transform(
		XMLoadFloat2(&s), scmat
	));

	mappedTransMatrix_->spSize = sps;

}

//!CAUTION:��]���Ή�
void
TextSpriteComponent::Draw() {
	if (owner_->GetState() != Object::Active) {
		return;
	}
	auto cmdList = dx12_->CommandList();
	//vb,ib�̐ݒ�
	cmdList->IASetVertexBuffers(0, 1, &(sprite_->VBView()));

	//�ʒu���transbuffer��descHeap�̐ݒ�
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1��rootparameter�ƕR�Â�
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());

	//texture��descheap�̐ݒ�
	ID3D12DescriptorHeap* texHeaps[] = {
		sprite_->TextureHeap()
	};
	cmdList->SetDescriptorHeaps(1, texHeaps);
	//2��rootparameter�ƕR�Â�
	cmdList->SetGraphicsRootDescriptorTable(
		2, sprite_->TextureHeap()->GetGPUDescriptorHandleForHeapStart());




	//TODO:Alligment�̕ύX�ɑΉ�����
	//TODO:���s�ɑΉ�����

	if (str_.size() > MAX_CLIP_SIZE) {
		//�ő啶�����ɍ��킹��
		str_.erase(MAX_CLIP_SIZE);
	}

	//������ǂݐi�߂����
	int k = (allign_ & ALLIGN_BUTTOM) ? -1 : 1;
	float x = 0; float y = 0;//�ǂݎ��ʒu
	int fx = 0;//�s�̐擪�͉������ڂ�
	int nx = 0;//���������ڂ�
	auto spSize = sprite_->GetSize();

	//�ǂݎn�߂�C���f�b�N�X�ԍ�
	int idx = (allign_ & ALLIGN_BUTTOM) ? str_.size() - 1 : 0;
	for (int i=0 ; i < str_.size(); i++) {

		if (str_[idx] == '\n') {//���s
			if (allign_ == ALLIGN_RIGHT ||//TOP����RIGHT
				allign_ == ALLIGN_BUTTOM) {//BUTTOM����LEFT
				//�s�����s�����炷
				for (int j = fx; j < nx; j++) {
					mappedTransMatrix_->instancePos[j].x -= x;
				}
			}
			y += k * spSize.y;
			fx = nx;
			x = 0;
			idx += k;
			continue;
		}

		//������uv���W�𓾂�
		int c = str_[idx];
		if (c < 32 || c >= 128) {
			continue;
		}
		c -= 32;//�摜�ɍ��킹��
		float u = 1.0f / X_CHAR_NUM;
		float v = 1.0f / Y_CHAR_NUM;
		Vector2 leftTop;
		leftTop.x = (c % X_CHAR_NUM) * u;
		leftTop.y = (c / X_CHAR_NUM) * v;
		mappedTransMatrix_->clipLeftTop[nx] = leftTop;

		//�����̍��W�𓾂�
		auto pos = Vector2(x, y);
		mappedTransMatrix_->instancePos[nx] =  pos;
		x += k * spSize.x;
		nx++;
		idx += k;
	}
	if (allign_ == ALLIGN_RIGHT ||//TOP����RIGHT
		allign_ == ALLIGN_BUTTOM) {//BUTTOM����LEFT
		//�s�����s�����炷
		for (int j = fx; j < nx; j++) {
			mappedTransMatrix_->instancePos[j].x -= x;
		}
	}


	//�`��
	cmdList->DrawInstanced(4, nx, 0, 0);
}

void
TextSpriteComponent::SetScaleRatioToWindowSize(const Vector2& ratio) {
	auto wsize = Game::Instance()->GetWindowSize();
	auto spSize = sprite_->GetSize();
	auto charSize = Vector2(spSize.x / X_CHAR_NUM,
		spSize.y / Y_CHAR_NUM);
	float kx = wsize.cx / charSize.x;
	scale_.x = kx * ratio.x;
	float ky = wsize.cy / charSize.y;
	scale_.y = ky * ratio.y;
}
void
TextSpriteComponent::SetScaleRatioToWindowSizeX(float ratio) {
	auto wsize = Game::Instance()->GetWindowSize();
	auto spSize = sprite_->GetSize();
	auto charSize = Vector2(spSize.x / X_CHAR_NUM,
		spSize.y / Y_CHAR_NUM);
	float kx = wsize.cx / charSize.x;
	scale_.x = kx * ratio;
	scale_.y = scale_.x;
}
void
TextSpriteComponent::SetScaleRatioToWindowSizeY(float ratio) {
	auto wsize = Game::Instance()->GetWindowSize();
	auto spSize = sprite_->GetSize();
	auto charSize = Vector2(spSize.x / X_CHAR_NUM,
		spSize.y / Y_CHAR_NUM);
	float ky = wsize.cy / charSize.y;
	scale_.y = ky * ratio;
	scale_.x = scale_.y;
}
