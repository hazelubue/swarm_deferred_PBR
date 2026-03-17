#if !defined INCLUDE_STR_LIGHT_REGENERATOR_H
#define INCLUDE_STR_LIGHT_REGENERATOR_H

#include "..\materialsystem\itexture.h"
#include "..\materialsystem\imaterialsystem.h"


class CDeferredLightDataRegenerator : public ITextureRegenerator {
public:
    float data[8];

    void RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect) override;

    void Release() override { delete this; }

    ITexture* m_pTex;
};

#endif