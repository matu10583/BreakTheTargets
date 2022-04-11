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
	//移動、回転情報の更新

	//今のままだとフォント画像のスケールになっているので
	//壱文字当たりのスケールに直す
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

	//表示する字のサイズを求める
	auto s = sprite_->GetSize();
	auto sps = Vector2(XMVector2Transform(
		XMLoadFloat2(&s), scmat
	));

	mappedTransMatrix_->spSize = sps;

}

//!CAUTION:回転未対応
void
TextSpriteComponent::Draw() {
	if (owner_->GetState() != Object::Active) {
		return;
	}
	auto cmdList = dx12_->CommandList();
	//vb,ibの設定
	cmdList->IASetVertexBuffers(0, 1, &(sprite_->VBView()));

	//位置情報transbufferのdescHeapの設定
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1番rootparameterと紐づけ
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());

	//textureのdescheapの設定
	ID3D12DescriptorHeap* texHeaps[] = {
		sprite_->TextureHeap()
	};
	cmdList->SetDescriptorHeaps(1, texHeaps);
	//2番rootparameterと紐づけ
	cmdList->SetGraphicsRootDescriptorTable(
		2, sprite_->TextureHeap()->GetGPUDescriptorHandleForHeapStart());




	//TODO:Alligmentの変更に対応する
	//TODO:改行に対応する

	if (str_.size() > MAX_CLIP_SIZE) {
		//最大文字数に合わせる
		str_.erase(MAX_CLIP_SIZE);
	}

	//文字を読み進める方向
	int k = (allign_ & ALLIGN_BUTTOM) ? -1 : 1;
	float x = 0; float y = 0;//読み取り位置
	int fx = 0;//行の先頭は何文字目か
	int nx = 0;//今何文字目か
	auto spSize = sprite_->GetSize();

	//読み始めるインデックス番号
	int idx = (allign_ & ALLIGN_BUTTOM) ? str_.size() - 1 : 0;
	for (int i=0 ; i < str_.size(); i++) {

		if (str_[idx] == '\n') {//改行
			if (allign_ == ALLIGN_RIGHT ||//TOPかつRIGHT
				allign_ == ALLIGN_BUTTOM) {//BUTTOMかつLEFT
				//行を改行分ずらす
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

		//文字のuv座標を得る
		int c = str_[idx];
		if (c < 32 || c >= 128) {
			continue;
		}
		c -= 32;//画像に合わせる
		float u = 1.0f / X_CHAR_NUM;
		float v = 1.0f / Y_CHAR_NUM;
		Vector2 leftTop;
		leftTop.x = (c % X_CHAR_NUM) * u;
		leftTop.y = (c / X_CHAR_NUM) * v;
		mappedTransMatrix_->clipLeftTop[nx] = leftTop;

		//文字の座標を得る
		auto pos = Vector2(x, y);
		mappedTransMatrix_->instancePos[nx] =  pos;
		x += k * spSize.x;
		nx++;
		idx += k;
	}
	if (allign_ == ALLIGN_RIGHT ||//TOPかつRIGHT
		allign_ == ALLIGN_BUTTOM) {//BUTTOMかつLEFT
		//行を改行分ずらす
		for (int j = fx; j < nx; j++) {
			mappedTransMatrix_->instancePos[j].x -= x;
		}
	}


	//描画
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
