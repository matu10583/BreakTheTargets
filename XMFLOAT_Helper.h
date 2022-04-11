#pragma once
//Vector3の拡張機能
#include <cmath>
#include <DirectXMath.h>
using namespace DirectX;


//XMFLOAT3
class Vector3:public XMFLOAT3
{
public:
    Vector3() = default;
    Vector3(float x, float y, float z) {
        this->x = x; this->y = y; this->z = z;
    }
    Vector3(const XMVECTOR& xmvec){
        XMVECTOR tmp = xmvec;
        XMStoreFloat3(this, tmp);
    }
    Vector3(const XMFLOAT3& xf) {
        this->x = xf.x; this->y = xf.y; this->z = xf.z;
    }
     inline void operator+= (const Vector3& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }

     inline void operator-= (const Vector3& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

     inline void operator*= (const float f)
    {
        x *= f;
        y *= f;
        z *= f;
    }

     inline void operator/= (const float f)
    {
        x /= f;
        y /= f;
        z /= f;
    }

     inline const auto operator-()const
    {
        return  Vector3(-x, -y, -z);
    }

     inline const auto operator+(
        const Vector3& v)const
    {
        return Vector3{ x + v.x, y + v.y, z + v.z };
    }

     inline const auto operator-(
         const Vector3& v)const
    {
        return Vector3{ x - v.x, y - v.y, z - v.z };
    }

     inline const auto operator*(
         const float f)const
    {
        return Vector3{ x * f, y * f, z * f };
    }

     inline const auto operator/ (
         const float f)const
    {
        return Vector3{ x / f, y / f, z / f };
    }

     //パディングでHLSL用変数はXMFLOAT4で用意してることが多いので
     //キャストをオーバーロードする
     inline operator XMFLOAT4() const {
         return XMFLOAT4(x, y, z, 1.0f);
     }

     



    //零ベクトルかどうかの判定
     inline bool ApproxZero()const {
        return
            (std::abs(x) < FLT_EPSILON) &&
            (std::abs(y) < FLT_EPSILON) &&
            (std::abs(z) < FLT_EPSILON);
    }



    inline float MagnitudeSqr()const {
        return x * x + y * y + z * z;
    }

    //大きさ
    inline float Magnitude()const {
        float sMag = MagnitudeSqr();
        return sqrt(sMag);
    }

    //正規化
    inline void Normalize() {
        *this /= Magnitude();
    }

    static inline auto Zero() {
        return Vector3(0, 0, 0);
    }


private:

};

//ドット積
static inline float Dot(const Vector3& v1, const Vector3& v2) {
    return
        v1.x * v2.x +
        v1.y * v2.y +
        v1.z * v2.z;
}

//クロス積
static inline auto Cross(const Vector3& v1, const Vector3& v2) {
    return Vector3(
        v1.y * v2.z - v2.y * v1.z,
        v1.z * v2.x - v2.z * v1.x,
        v1.x * v2.y - v2.x * v1.y
    );
}




//XMFLOAT2
class Vector2 :public XMFLOAT2
{
public:
    Vector2() = default;
    Vector2(float x, float y) {
        this->x = x; this->y = y;
    }
    Vector2(const XMVECTOR& xmvec) {
        XMVECTOR tmp = xmvec;
        XMStoreFloat2(this, tmp);
    }
    Vector2(const XMFLOAT2& xf) {
        this->x = xf.x; this->y = xf.y;
    }
    inline void operator+= (const Vector2& v)
    {
        x += v.x;
        y += v.y;
    }

    inline void operator-= (const Vector2& v)
    {
        x -= v.x;
        y -= v.y;
    }

    inline void operator*= (const float f)
    {
        x *= f;
        y *= f;
    }

    inline void operator/= (const float f)
    {
        x /= f;
        y /= f;
    }

    inline const auto operator-()const
    {
        return  Vector2(-x, -y);
    }

    inline const auto operator+(
        const Vector2& v)const
    {
        return Vector2{ x + v.x, y + v.y};
    }

    inline const auto operator-(
        const Vector2& v)const
    {
        return Vector2{ x - v.x, y - v.y};
    }

    inline const auto operator*(
        const float f)const
    {
        return Vector2{ x * f, y * f};
    }

    inline const auto operator/ (
        const float f)const
    {
        return Vector2{ x / f, y / f};
    }



    //零ベクトルかどうかの判定
    inline bool ApproxZero()const {
        return
            (std::abs(x) < FLT_EPSILON) &&
            (std::abs(y) < FLT_EPSILON);
    }



    inline float MagnitudeSqr()const {
        return x * x + y * y;
    }

    //大きさ
    inline float Magnitude()const {
        float sMag = MagnitudeSqr();
        return sqrt(sMag);
    }

    //正規化
    inline void Normalize() {
        *this /= Magnitude();
    }


    static inline auto Zero() {
        return Vector2(0, 0);
    }



private:

}; 

static inline float Dot(const XMFLOAT2& v1, const XMFLOAT2& v2) {
    return
        v1.x * v2.x +
        v1.y * v2.y;
}

//頂点情報用
struct VertexData
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;
};
struct VertexData2D
{
    Vector3 pos;
    Vector2 uv;
};
//インデックスバッファー用
struct Index
{
    Index():
    a(0),b(0), c(0)
    {}
    Index(uint16_t xa, uint16_t xb, uint16_t xc) {
        a = xa; b = xb; c = xc;
    }
    uint16_t a;
    uint16_t b;
    uint16_t c;
};


