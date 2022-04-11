#include "Sprite.h"
Sprite::Sprite():
ibView_(),
vbView_()
{

}

const Vector2
Sprite::GetSize()const {
	auto desc = textureImg_->GetDesc();
	auto w = static_cast<float>(desc.Width);
	auto h = static_cast<float>(desc.Height);
	auto ret = Vector2(w, h);
	return ret;
}